<?php
/**
 * Field handler to provide a list of template node types.
 */
class flag_lists_handler_field_template_types extends views_handler_field_prerender_list {
  function construct() {
    parent::construct();
    $this->additional_fields['name'] = array('table' => 'flags', 'field' => 'name');
  }
  function query() {
    $this->add_additional_fields();
    $this->field_alias = $this->aliases['name'];
  }

  function pre_render($values) {
    $names = array();
    $this->items = array();

    foreach ($values as $result) {
      $names[] = $result->{$this->aliases['name']};
    }

    if ($names) {
      $result = db_query("SELECT flt.type, flt.name FROM {flag_lists_types} flt INNER JOIN {flags} f ON flt.name = f.name WHERE f.name IN ('" . implode('\', \'', $names) . "') ORDER BY flt.type");
      while ($type = db_fetch_object($result)) {
        $this->items[$type->name][$type->type] = check_plain($type->type);
      }
    }
  }
}
