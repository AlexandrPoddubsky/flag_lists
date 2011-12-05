<?php
/**
 * @file flag_lists_handler_field_template_types.inc
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
      $query = db_select('flag_lists_types', 'flt');
      $query->innerJoin('flags', 'f', 'flt.name = f.name');
      $result = $query->fields('flt', array('type', 'name'))
        ->condition('f.name', $names, 'IN')
        ->orderBy('flt.type')
        ->execute();
      foreach ($result as $type) {
        $this->items[$type->name][$type->type] = check_plain($type->type);
      }
    }
  }
}
