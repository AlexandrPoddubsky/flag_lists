<?php
/**
 * @file
 * Provide list fid argument handler.
 */

/**
 * Argument handler to accept a list id.
 */
class flag_lists_handler_argument_fid extends views_handler_argument_numeric {
  /**
   * Override the behavior of title(). Get the title of the list.
   */
  function title_query() {
    $titles = array();
    $placeholders = implode(', ', array_fill(0, sizeof($this->value), '%d'));

    $result = db_query("SELECT fl.title FROM {flag_lists_flags} fl WHERE fl.fid IN ($placeholders)", $this->value);
    while ($term = db_fetch_object($result)) {
      $titles[] = check_plain($term->title);
    }
    return $titles;
  }
}

