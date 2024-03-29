<?php 
// $Id: evanced_events_importer.xml2node.inc,v 1.1.2.1 2010/09/17 17:26:36 slanger Exp $

/**
 * @file
 * When cron is run, nodes are added, updated or deleted based on an EXML feed.
 *
 * The following functions are activated when cron runs. Data is extracted from 
 * an EXML feed provided by the software package Evanced Events. Based on that 
 * data, event nodes are then either added, updated or deleted in Drupal.
 */

/**
 * Finds taxonomy terms that should be associated with the current event data.
 *
 * @param $node_values_array
 *   An array containing event data that will be used to create a Drupal node.
 *
 * @return
 *   An array of taxonomy term ids.
 */
function evanced_events_importer_taxonomy_ids($node_values_array) {

  $term_mapping_array = array();
  $taxonomy_ids_array = array();

  // Retrieve any taxonomy configuration settings
  // from the database and load them as an object.
  $xmlmapper_config_settings = evanced_events_importer_load_configuration('evanced_events_importer_xmlmapper', 'taxonomy');

  // Make a loop for each XML element that has been
  // identified as representing taxonomy data.
  foreach ($xmlmapper_config_settings as $xmlmapper_object) {

    // Reset the XML-to-vocabulary array.
    $xml_to_vocab_array = array();

    // Split the XML elements into an array, using the
    // comma as a delimiter, and then loop through them.
    foreach (explode(",", $xmlmapper_object->xml_elements) as $object_key => $object_xml_elements) {
      // Append an array with the XML element name and
      // the ID of the vocabulary to which it is associated.
      $xml_to_vocab_array[$object_key]['xml_element_name'] = trim($object_xml_elements);
      $xml_to_vocab_array[$object_key]['vocab_id'] = str_replace('taxonomy', '', trim($xmlmapper_object->node_field_name));
    }

    // Merge the current array with the main taxonomy array.
    $term_mapping_array = array_merge((array)$xml_to_vocab_array, (array)$term_mapping_array);

  }

  // The following query retrieves any term ID where either
  // the name or the synonym matches the evanced category name.
  $sql = "SELECT  t.tid FROM {term_data} t WHERE  t.vid = %d AND  LOWER(t.name) = LOWER('%s') " .
         " UNION " .
         "SELECT  t.tid FROM {term_synonym} t WHERE  LOWER(t.name) = LOWER('%s') ";

  // Make a loop for each XML element that has been mapped.
  foreach ($term_mapping_array as $array_key => $term_mapping_array) {

    // Run the query, which retrieves term
    // ids associated with the current event.
    $result = db_query($sql, $term_mapping_array['vocab_id'],
                        trim($node_values_array[$term_mapping_array['xml_element_name']]),
                        trim($node_values_array[$term_mapping_array['xml_element_name']]));

    // Make loop for each term ID in the query result.
    while ($data = db_fetch_object($result)) {
      // Append the term ID to the master taxonomy array.
      $taxonomy_ids_array[] = $data->tid;
    }

  }

  // Return the array of taxonomy term ids.
  return $taxonomy_ids_array;

}


/**
 * Processes offsite address data and adds it to the main array.
 *
 * @param $node_values_array
 *   An array containing all the event data that
 *   will be used to create a single Drupal node.
 *
 * @return
 *   An updated array of event data.
 */
function evanced_events_importer_offsite_address($node_values_array) {

  $element_address = '';

  // Retrieve any address configuration settings
  // from the database and load them as an object.
  $xmlmapper_config_settings = evanced_events_importer_load_configuration('evanced_events_importer_xmlmapper', 'addresses_cck');

  // Set the address-related XML elements as a variable.
  $element_address = $xmlmapper_config_settings[0]->xml_elements;

  // Determine if any addresses data exists.
  if (trim($node_values_array[$element_address]) != '') {

    // Replace any incorrectly-formatted line break tags with the correct version.
    $node_values_array[$element_address] = str_replace(chr(60) . 'br' . chr(62),  '<br />', $node_values_array[$element_address]);
    $node_values_array[$element_address] = str_replace('<br/>', '<br />', $node_values_array[$element_address]);

    // Split the address string into an array,
    // using the html line breaks as a delimiter.
    $address_offsite_array = explode('<br />', $node_values_array[$element_address]);

    // Separate the city from the last line of the address.
    $city_state_zip_array = explode(',', trim($address_offsite_array[(count($address_offsite_array) - 1)]));

    // Separate the state and zip from each other.
    $state_zip_array = explode(' ', trim($city_state_zip_array[1]));

    // Set the address components as variables.
    $node_values_array['offsite_location_name'] = trim($address_offsite_array[0]);
    $node_values_array['street1']               = $address_offsite_array[1];
    $node_values_array['city']                  = $city_state_zip_array[0];
    $node_values_array['state']                 = 'OH';
    $node_values_array['zip']                   = $state_zip_array[1];

  // Otherwise, the address-related field is empty.
  }
  else {

    // Ensure that any offsite location information that
    // previously existed in the database gets cleared out.
    $node_values_array['offsite_location_name'] = '';
    $node_values_array['street1']               = '';
    $node_values_array['city']                  = '';
    $node_values_array['state']                 = '';
    $node_values_array['zip']                   = '';

  }

  return $node_values_array;

}


/**
 * Processes the event's date/time values and adds them to the main array.
 *
 * @param $node_values_array
 *   An array containing all the event data that
 *   will be used to create a single Drupal node.
 *
 * @return
 *   An updated array of event data.
 */
function evanced_events_importer_event_dates($node_values_array) {

  $element_date      = '';
  $element_starttime = '';
  $element_endtime   = '';

  // Retrieve any date configuration settings
  // from the database and load them as an object.
  $xmlmapper_config_settings = evanced_events_importer_load_configuration('evanced_events_importer_xmlmapper', 'date');

  // Make a loop for each date XML element already
  // mapped to a content type field in the database.
  foreach ($xmlmapper_config_settings as $xmlmapper_object) {

    // Determine if the current XML element pertains to a date.
    if (preg_match('/\_date$/', trim($xmlmapper_object->node_field_name_alias))) {
      // Set the current XML element as the date key.
      $element_date = $xmlmapper_object->xml_elements;
    } 
    // Otherwise, determine if the current XML element pertains to the start time.
    elseif (preg_match('/\_starttime$/', trim($xmlmapper_object->node_field_name_alias))) {
      // set the current XML element as the start time key.
      $element_starttime = $xmlmapper_object->xml_elements;
    } 
    // Otherwise, determine if the current XML element pertains to the end time.
    elseif (preg_match('/\_endtime$/', trim($xmlmapper_object->node_field_name_alias))) {
      // Set the current XML element as the end time key.
      $element_endtime = $xmlmapper_object->xml_elements;
    }

  }

  // Determine if the current event lasts all day, with no specific start or end time.
  if ($node_values_array[$element_starttime] == 'All Day') {
    // Clear the start and end time values.
    $node_values_array[$element_starttime] = '';
    $node_values_array[$element_endtime]   = '';
  }

  // Assemble the start date/time and ending date/time,
  // using the date components from the XML feed.
  $datetime_start = trim($node_values_array[$element_date] . ' ' . $node_values_array[$element_starttime]);
  $datetime_end   = trim($node_values_array[$element_date] . ' ' . $node_values_array[$element_endtime]);

  // Set the starting and ending dates as variables, formatting them for the database.
  $node_values_array['event_date_from'] = date('Y-m-d\TH:i:s', strtotime($datetime_start));
  $node_values_array['event_date_to']   = date('Y-m-d\TH:i:s', strtotime($datetime_end));

  return $node_values_array;

}


/**
 * Determines if a date/time value is larger than any processed yet so far.
 *
 * @param $datetime_start
 *   The starting date of the current event.
 *
 * @param $xml_maxdate
 *   The maximum date that has been processed so far.
 *
 * @return
 *   The largest date value that was found.
 */
function evanced_events_importer_find_maximum_feed_date($datetime_start, $xml_maxdate) {

  // Determine if the current start date happens
  // later than any other date processed so far.
  if (strtotime($datetime_start) > $xml_maxdate) {
    // Update the value of the maximum date.
    $xml_maxdate = strtotime($datetime_start);
  }

  return $xml_maxdate;

}


/**
 * Determines if a date/time value is smaller than any processed yet so far.
 *
 * @param $datetime_start
 *   The starting date of the current event.
 *
 * @param $xml_mindate
 *   The minimum date that has been processed so far.
 *
 * @return
 *   The smallest date value that was found.
 */
function evanced_events_importer_find_minimum_feed_date($datetime_start, $xml_mindate) {

  // Determine if the new start date happens
  // earlier than any other date processed so far.
  if (strtotime($datetime_start) < $xml_mindate) {
    // Update the value of the minimum date.
    $xml_mindate = strtotime($datetime_start);
  }

  return $xml_mindate;

}


/**
 * Uses an array of event data to create a new node or edit an existing one.
 *
 * @param $nid
 *   The id of the node that needs to be updated. 
 *   If it's zero, a new node will be created.
 *
 * @param $node_values_array
 *   An array containing all the event data that
 *   will be used to create a single Drupal node.
 *
 * @param $taxonomy_term_array
 *   An array of taxonomy ids associated with the current event.
 */
function evanced_events_importer_process_node($nid, $node_values_array, $taxonomy_term_array) {

  // Retrieve any existing configuration settings
  // from the database and load them as an object.
  $main_config_settings = evanced_events_importer_load_configuration('evanced_events_importer');
  $xmlmapper_config_settings = evanced_events_importer_load_configuration('evanced_events_importer_xmlmapper');

  // Determine if a node ID was found. That means an
  // existing event will be updated (rather than added).
  if (($nid != 0) && ($nid != '')) {
    // Load the event node and set it as an object.
    $node = node_load($nid);
  // Otherwise, no node ID was retrieved. This event
  // will be added to Drupal for the first time.
  }
  else {
    // Create an object for the new node.
    $node = new stdClass();
  }

  // Retrieve the content type to which this data will be
  // added, as well as the ID of the publishing author.
  $node->type = $main_config_settings[0]->content_type_name;
  $node->uid  = $main_config_settings[0]->publisher_uid;

  // Ensure that the current node is published.
  $node->status  = 1;
  $node->promote = 0;
  $node->sticky  = 0;

  // Ensure the 'created' date matches the
  // evanced 'last updated' value (note: Drupal
  // won't allow the 'changed' date to be overwritten).
  $node->created = strtotime(str_replace('Z', '', $node_values_array['lastupdated']));

  // Specify the correct input type for the node body.
  $node->format = $main_config_settings[0]->input_format;

  // Make a loop for each date XML element already
  // mapped to a content type field in the database.
  foreach ($xmlmapper_config_settings as $xmlmapper_object) {

    // Determine if this is a date field.
    if ($xmlmapper_object->node_field_type == 'date') {
      // Add the start and end dates to the node object.
      $node->{$xmlmapper_object->node_field_name}[0]['value']  = $node_values_array['event_date_from'];
      $node->{$xmlmapper_object->node_field_name}[0]['value2'] = $node_values_array['event_date_to'];
    } 
    // Otherwise, determine if this is an addresses field.
    elseif ($xmlmapper_object->node_field_type == 'addresses_cck') {
      // Add the offsite address data to the node object.
      $node->{$xmlmapper_object->node_field_name}[0]['aname']       = trim($node_values_array['offsite_location_name']);
      $node->{$xmlmapper_object->node_field_name}[0]['street']      = trim($node_values_array['street1']);
      $node->{$xmlmapper_object->node_field_name}[0]['additional']  = trim($node_values_array['street2']);
      $node->{$xmlmapper_object->node_field_name}[0]['city']        = trim($node_values_array['city']);
      $node->{$xmlmapper_object->node_field_name}[0]['province']    = trim($node_values_array['state']);
      $node->{$xmlmapper_object->node_field_name}[0]['postal_code'] = trim($node_values_array['zip']);
    } 
    // Otherwise, determine if this is a taxonomy field.
    elseif ($xmlmapper_object->node_field_type == 'taxonomy') {
    }
    // Otherwise, this is probably a less 'widget-y' field.
    else {

      // Determine if this field is the node title.
      if (($xmlmapper_object->node_field_type == 'text') &&
          ($xmlmapper_object->node_field_name == 'title')) {
      // Add the node title to the node object.
        $node->{$xmlmapper_object->node_field_name} = trim($node_values_array[$xmlmapper_object->xml_elements]);
      }
      // Otherwise, determine if this field is the node body.
      elseif (($xmlmapper_object->node_field_type == 'text') &&
              ($xmlmapper_object->node_field_name == 'body')) {

        // Determine if the user wants all the html
        // links stripped from the event description.
        if ($main_config_settings[0]->striplinks_flag == 1) {
          // Remove all html links. instead, show the URL next to the link text.
          $node_values_array[$xmlmapper_object->xml_elements] = preg_replace('/<a[^>]*?href=[\'"](.*?)[\'"][^>]*?>(.*?)<\/a>/si', '$2', $node_values_array[$xmlmapper_object->xml_elements]);
        }

        // Add the node body to the node object.
        $node->{$xmlmapper_object->node_field_name} = trim($node_values_array[$xmlmapper_object->xml_elements]);
      }
      // Otherwise, this is probably a basic CCK field.
      else {
        // Add the current field to the node object.
        $node->{$xmlmapper_object->node_field_name}[0]['value'] = trim($node_values_array[$xmlmapper_object->xml_elements]);
      }

    }

  }

  // Add any taxonomy terms to the node object.
  $node->taxonomy = $taxonomy_term_array;  // term ids associated with this content.

  // Update Drupal with the changes, whether a new
  // node is being added or an old one is being edited.
  node_save($node);

  // Set the node ID as a variable.
  $nid = $node->nid;

}


/**
 * Parses the EXML feed, creating an array of event data for a single node.
 *
 * @param $item
 *   An object containing data for a single 
 *   event, retrieved from the EXML feed.
 *
 * @param $xml_nodes_array
 *   An array containing the unique XML
 *   tag names found in the EXML feed.
 *
 * @return
 *   An array containing all the event data that
 *   will be used to create a single Drupal node.
 */
function evanced_events_importer_populate_node_array($item, $xml_nodes_array) {

  // Make a loop for each element in the array.
  foreach ($xml_nodes_array as $node_key => $node_name) {

    // Retrieve data associated with this XML element.
    $node_element = $item->getElementsByTagName($node_name);
    // Add the value of the current element to an
    // array, using the XML tag to dynamically name it.
    $node_values_array[$node_name] = $node_element->item(0)->nodeValue;

  }

  return $node_values_array;

}


/**
 * Determines if an existing event node should be updated with new data.
 *
 * @param $id
 *   The unique ID of the event in Evanced.
 *
 * @param $lastupdated
 *   The date that the event was last updated in Evanced.
 *
 * @param $nid
 *   The id of the event node in Drupal.
 *
 * @param $update_database_flag
 *   A flag to signal whether or not the node should be updated.
 *
 * @return
 *   TRUE if the node should be updated; FALSE if it should be skipped.
 */
function evanced_events_importer_select_node_action($id, $lastupdated, &$nid, &$update_database_flag) {

  $nid = 0;
  $last_updated_db = '';
  $fieldname_evanced_id = '';
  $update_database_flag = TRUE;

  // Retrieve any existing configuration settings
  // from the database and load them as an object.
  $main_config_settings = evanced_events_importer_load_configuration('evanced_events_importer');
  $xmlmapper_config_settings = evanced_events_importer_load_configuration('evanced_events_importer_xmlmapper');

  // Make a loop for each row of mapped-field data.
  foreach ($xmlmapper_config_settings as $xmlmapper_object) {

    // Determine if the XML element associated with this field is the evanced id.
    if (drupal_strtolower(trim($xmlmapper_object->xml_elements)) == 'id') {
      // Set the evanced id's CCK field name as a variable.
      $fieldname_evanced_id = $xmlmapper_object->node_field_name;
      break;
    }

  }

  // Ensure that the evanced id's CCK field
  // name was found before continuing.
  if (trim($fieldname_evanced_id) != '') {

    // This sql query retrieves an event that
    // matches the evanced ID currently in play.
    $sql = "SELECT  ct.nid, n.created " .
            " FROM  {content_type_%s} ct, {node} n " .
            "WHERE  ct.nid = n.nid " .
            "  AND  n.type = '%s' " .
            "  AND  ct.%s_value = '%s' ";

    // Run the query.
    $result = db_query($sql, trim($main_config_settings[0]->content_type_name), 
                        trim($main_config_settings[0]->content_type_name), 
                        trim($fieldname_evanced_id), trim($id));

    // Make loop for each record in the query results.
    while ($data = db_fetch_object($result)) {
      $nid = $data->nid;
      $last_updated_db = date('Y-m-d\TH:i:s\Z', $data->created);
      // $last_updated_db = date('Y-m-d\TH:i:s\Z', strtotime('+1 hour', $data->created));
    }

    // Determine if the 'last updated' date for this
    // record matches in both Drupal and evanced.
    if ($last_updated_db == $lastupdated) {
      // This record hasn't changed. Skip it.
      $update_database_flag = FALSE;
    }

  }
  // Otherwise, the evanced ID wasn't found
  // and the database query didn't run.
  else {
    // There's not enough valid
    // information to update this record.
    $update_database_flag = FALSE;
  }

}


/**
 * Deletes event nodes in Drupal that should exist in the EXML feed, but don't.
 *
 * @param $valid_evanced_ids
 *   A comma-delimited string of Evanced IDs found in the EXML feed.
 *
 * @param $xml_mindate
 *   The minimum event date found in the EXML feed.
 *
 * @param $xml_maxdate
 *   The maximum event date found in the EXML feed.
 *
 * @todo Sometimes, this function erroneously deletes event nodes,
     which exist the day before the earliest event in EXML feed.
 */
function evanced_events_importer_delete_events($valid_evanced_ids, $xml_mindate, $xml_maxdate) {

  $fieldname_date = '';
  $fieldname_evanced_id = '';

  // Retrieve any existing configuration settings
  // from the database and load them as an object.
  $main_config_settings = evanced_events_importer_load_configuration('evanced_events_importer');
  $xmlmapper_config_settings = evanced_events_importer_load_configuration('evanced_events_importer_xmlmapper');

  // Make a loop for each row of mapped-field data.
  foreach ($xmlmapper_config_settings as $xmlmapper_object) {

    // Determine if the XML element associated with this field is the evanced id.
    if (drupal_strtolower(trim($xmlmapper_object->xml_elements)) == 'id') {
      // Set the evanced id's CCK field name as a variable.
      $fieldname_evanced_id = $xmlmapper_object->node_field_name;
    }

    // Determine if this is a CCK date field.
    if (drupal_strtolower(trim($xmlmapper_object->node_field_type)) == 'date') {
      // Set the CCK date field name as a variable.
      $fieldname_date = $xmlmapper_object->node_field_name;
    }

    // Determine if both field names were found.
    if ((trim($fieldname_evanced_id) != '') &&
        (trim($fieldname_date)       != '')) {
      break;
    }

  }

  // Ensure the CCK field names for both the evanced id
  // and the date field were found before continuing.
  if ((trim($fieldname_evanced_id) != '') &&
      (trim($fieldname_date)       != '')) {

    $date_table_alias = "ct";
    $date_table_name  = "";
    $date_table_join  = "";

    // Explode the string of valid Evanced IDs into an array.
    $valid_evanced_ids_array = explode(',', trim($valid_evanced_ids));

    // Generate placeholders for the array and return 
    // the correct %-placeholders to embed into a query.
    $placeholders = db_placeholders($valid_evanced_ids_array, 'int');

    // Determine if the event's start and end times are being stored in their
    // own database table, rather than inside the event's content type table.
    if (db_table_exists("content_" . trim($fieldname_date))) {

      // Ensure that the separate event date table
      // is included in the forthcoming sql query.
      $date_table_alias = "dt";
      $date_table_name  = "{content_" . trim($fieldname_date) . "} " . $date_table_alias . ", ";
      $date_table_join  = "AND  " . $date_table_alias . ".nid = n.nid ";

    }

    // This sql query retrieves any events that have a
    // start date falling within the timespan of the xml
    // feed, with an evanced ID that does *not* exist in the
    // XML file. This signals that the event should be deleted.
    $sql = "SELECT  ct.nid, n.created, ct.%s_value " .
            " FROM  {content_type_%s} ct, " .
                    $date_table_name . " {node} n " .
            "WHERE  ct.nid  = n.nid " .
                    $date_table_join .
            "  AND  n.type  = '%s' " .
            "  AND (ct.%s_value <> '' " .
            "  AND  ct.%s_value IS NOT NULL) " .
            "  AND (ct.%s_value NOT IN (%s)) " .
            "  AND (%s.%s_value BETWEEN '%s' AND '%s') ";

    // Run the query.
    $result = db_query($sql, trim($fieldname_evanced_id), trim($main_config_settings[0]->content_type_name),
                        trim($main_config_settings[0]->content_type_name), trim($fieldname_evanced_id),
                        trim($fieldname_evanced_id), trim($fieldname_evanced_id), $valid_evanced_ids,
                        $date_table_alias, trim($fieldname_date), date('Y-m-d\TH:i:s', $xml_mindate),
                        date('Y-m-d\TH:i:s', $xml_maxdate));

    // Make loop for each record in the query results.
    while ($data = db_fetch_object($result)) {
      // Delete the current event node from Drupal.
      node_delete($data->nid);
    }

  }

}


/**
 * Reads the EXML feed, parses the data and uses it to create/update nodes.
 */
function evanced_events_importer_parse_xml() {

  // Ensure that user information is globally accessible.
  global $user;

  $nid = 0;
  $xml_maxdate = 0;
  $xml_mindate = 100000000000000000000000;
  $evanced_ids_string   = '';
  $secondary_node_name  = '';
  $update_database_flag = TRUE;

  // Retrieve any existing configuration settings
  // from the database and load them as an object.
  $main_config_settings = evanced_events_importer_load_configuration('evanced_events_importer');

  // Ensure an XML feed url has been specified before continuing.
  if (trim($main_config_settings[0]->exml_feed_url) != '') {

    // Initialize the XML parser (Document Object Model (DOM) library).
    $doc = new DOMDocument();

    // Load the XML file.
    $doc->load($main_config_settings[0]->exml_feed_url);

    // Retrieve the root element in the XML file and set it as an object.
    $root_element = $doc->getElementsByTagName("*");

    // Populate an array with each unique
    // XML tag name found in the EXML feed.
    $xml_nodes_array = evanced_events_importer_xml_tag_names($root_element, $secondary_node_name);

    // Get each 'item' XML element.
    $items = $doc->getElementsByTagName($secondary_node_name);

    // Loop through all data contained within the <item> element.
    foreach ($items as $item) {

      // Populate an array with data from the EXML feed,
      // using the XML element name as the array key.
      $node_values_array = evanced_events_importer_populate_node_array($item, $xml_nodes_array);

      // Process event date data in the xml
      // feed and add it to the main array.
      $node_values_array = evanced_events_importer_event_dates($node_values_array);

      // Process offsite address data in the
      // XML feed and add it to the main array.
      $node_values_array = evanced_events_importer_offsite_address($node_values_array);

      // Retrieve taxonomy ids associated with
      // this event and set them as an array.
      $taxonomy_term_array = evanced_events_importer_taxonomy_ids($node_values_array);

      // Determine if a domain was specified on the configuration page, one that should
      // be converted into a relative link when found within the body of the event node.
      if (trim($main_config_settings[0]->relative_links_domain) != '') {
        // Convert any full links in the body of this document into relative links.
        $node_values_array['description'] = preg_replace('/href=("|\')http:\/\/' .
                                                          trim($main_config_settings[0]->relative_links_domain) . '\//si',
                                                         'href=$1<?php print base_path(); ?>',
                                                          $node_values_array['description']);
      }

      // Determine whether the current date happens
      // later than any other date processed so far.
      $xml_maxdate = evanced_events_importer_find_maximum_feed_date($node_values_array['event_date_from'], $xml_maxdate);

      // Determine whether the current date happens
      // earlier than any other date processed so far.
      $xml_mindate = evanced_events_importer_find_minimum_feed_date($node_values_array['event_date_from'], $xml_mindate);

      // Append the current evanced ID to the comma-delimited string.
      $evanced_ids_string .= ',' . $node_values_array['id'];

      // Determine whether an existing event node in
      // Drupal needs to be updated or should be skipped.
      evanced_events_importer_select_node_action($node_values_array['id'], $node_values_array['lastupdated'], $nid, $update_database_flag);

      // Ensure the database should be updated before continuing.
      if ($update_database_flag) {
        // Update Drupal with the processed EXML
        // data by either adding or editing a node.
        evanced_events_importer_process_node($nid, $node_values_array, $taxonomy_term_array);
      }

    }

    // Remove the first occurrence of the comma in the string.
    $evanced_ids_string = ltrim(trim($evanced_ids_string), ',');

    // Reformat any existing dates so that they are compatible with the database.
    $today_date_object = mktime(0, 0, 0, date("m"), date("d"), date("Y"));
    $xml_mindate = strtotime(date('Y-m-d\TH:i:s', $today_date_object));
    $xml_maxdate = strtotime(date('Y-m-d\T11:59:59', $today_date_object) . ' +365 days');

    // Ensure at least one evanced ID exists
    // within the string before continuing.
    if (trim($evanced_ids_string) != '') {
      // Delete any event nodes that should exist
      // within the current XML feed, but don't.
      evanced_events_importer_delete_events($evanced_ids_string, $xml_mindate, $xml_maxdate);
    }

  }

}
