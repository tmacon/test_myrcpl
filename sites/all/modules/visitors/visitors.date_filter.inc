<?php

// $Id: visitors.date_filter.inc,v 1.2 2010/02/26 13:00:14 gashev Exp $

/**
 * @file
 * Date filter form for the visitors module.
 */

/**
 * Retrieves a date filter form from a constructor function, or from the cache
 * if the form was built in a previous page-load. The form is then passed on
 * for processing, after and rendered for display if necessary.
 */
function visitors_date_filter() {
  return drupal_get_form('visitors_date_filter_form');
}

/**
 * Set to session info default values for visitors date filter.
 */
function visitors_set_session_date_range() {
  if (!isset($_SESSION['visitors_from'])) {
    $_SESSION['visitors_from'] = array(
      'day'   => 1,
      'month' => date('n'),
      'year'  => date('Y'),
    );
  }

  if (!isset($_SESSION['visitors_to'])) {
    $_SESSION['visitors_to'] = array(
      'day'   => date('j'),
      'month' => date('n'),
      'year'  => date('Y'),
    );
  }
}

/**
 * Get year range for date fields visitors date filter.
 * Min year - min value from {visitors} table.
 *            If min value does not exist -  use current year.
 * Max year - current year.
 *
 * @return int array years array from min year to max year
 */
function visitors_get_years_range() {
  $sql = sprintf('SELECT MIN(visitors_date_time) AS min_time
                  FROM {visitors}
                  LIMIT 1'
                );
  $result = db_query($sql);
  $min_time = time();
  while ( $data = db_fetch_object($result) ) {
    $min_time = $data->min_time;
  }

  return range(date('Y', $min_time), date('Y'));
}

/**
 * Roll out a single date element.
 */
function visitors_expand_date($element) {
  // Default to current date
  if (empty($element['#value'])) {
    $element['#value'] = array('day' => format_date(time(), 'custom', 'j'),
                            'month' => format_date(time(), 'custom', 'n'),
                            'year' => format_date(time(), 'custom', 'Y'));
  }

  $element['#tree'] = TRUE;

  // Determine the order of day, month, year in the site's chosen date format.
  $format = variable_get('date_format_short', 'm/d/Y - H:i');
  $sort = array();
  $sort['day'] = max(strpos($format, 'd'), strpos($format, 'j'));
  $sort['month'] = max(strpos($format, 'm'), strpos($format, 'M'));
  $sort['year'] = strpos($format, 'Y');
  asort($sort);
  $order = array_keys($sort);

  // Output multi-selector for date.
  foreach ($order as $type) {
    switch ($type) {
      case 'day':
        $options = drupal_map_assoc(range(1, 31));
        break;
      case 'month':
        $options = drupal_map_assoc(range(1, 12), 'map_month');
        break;
      case 'year':
        $options = drupal_map_assoc(visitors_get_years_range());
        break;
    }
    $parents = $element['#parents'];
    $parents[] = $type;
    $element[$type] = array(
      '#type' => 'select',
      '#value' => $element['#value'][$type],
      '#attributes' => $element['#attributes'],
      '#options' => $options,
    );
  }

  return $element;
}

/**
 * Date filter form data.
 */
function visitors_date_filter_form($form_state) {
  visitors_set_session_date_range();

  $form = array();

  $form['visitors_date_filter'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Date filter'),
    '#collapsible'   => FALSE,
    '#collapsed'     => FALSE,
    '#description'   => t('Choose date range')
  );

  $form['visitors_date_filter']['from'] = array(
    '#type'          => 'date',
    '#title'         => t('From'),
    '#process'       => array('visitors_expand_date'),
    '#default_value' => $_SESSION['visitors_from']
  );

  $form['visitors_date_filter']['to'] = array(
    '#type'          => 'date',
    '#title'         => t('To'),
    '#process'       => array('visitors_expand_date'),
    '#default_value' => $_SESSION['visitors_to']
  );

  $form['visitors_date_filter']['submit'] = array(
    '#type'          => 'submit',
    '#value'         => t('Submit'),
  );

  return $form;
}

/**
 * Visitors date filter form values validation.
 */
function visitors_date_filter_form_validate($form, &$form_state) {
  $from = mktime(0, 0, 0,
    $form_state['values']['from']['month'],
    $form_state['values']['from']['day'],
    $form_state['values']['from']['year']
  );

  $to  = mktime(23, 59, 59,
    $form_state['values']['to']['month'],
    $form_state['values']['to']['day'],
    $form_state['values']['to']['year']
  );

  if ((int) $from <= 0) {
    return form_set_error('from', t('The specified date is invalid.'));
  }

  if ((int) $to <= 0) {
    return form_set_error('to', t('The specified date is invalid.'));
  }

  if ($from > $to) {
    form_set_error('from', t('The specified date range is invalid.'));
  }
}

/**
 * Submit values from date filter form write to session info.
 */
function visitors_date_filter_form_submit($form, &$form_state) {
  $_SESSION['visitors_from'] = $form_state['values']['from'];
  $_SESSION['visitors_to']   = $form_state['values']['to'];
}

/**
 * Convert from date value to timestamp.
 *
 * @return returns the Unix timestamp of the session arguments given.
 *         If the arguments are invalid, the function returns FALSE
 *         (before PHP 5.1 it returned -1).
 */
function visitors_get_from_timestamp() {
  return mktime(0, 0, 0,
                $_SESSION['visitors_from']['month'],
                $_SESSION['visitors_from']['day'],
                $_SESSION['visitors_from']['year']
  );
}

/**
 * Convert to date value to timestamp.
 *
 * @return returns the Unix timestamp of the session arguments given.
 *         If the arguments are invalid, the function returns FALSE
 *         (before PHP 5.1 it returned -1).
 */
function visitors_get_to_timestamp() {
  return mktime(23, 59, 59,
                $_SESSION['visitors_to']['month'],
                $_SESSION['visitors_to']['day'],
                $_SESSION['visitors_to']['year']
  );
}

/**
 * Build sql query from date filter values.
 *
 * @return string sql query.
 */
function visitors_date_filter_sql_condition() {
  visitors_set_session_date_range();

  $from = visitors_get_from_timestamp();
  $to   = visitors_get_to_timestamp();

  return sprintf('visitors_date_time BETWEEN %d AND %d', $from, $to);
}
