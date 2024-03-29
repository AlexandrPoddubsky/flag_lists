<?php

/**
 * @file
 * Contains administrative pages for creating, editing, and deleting flag lists.
 */

/**
 * Flag administration page. Display a list of existing flags.
 */
function flag_lists_admin_page() {
  return theme('flag_lists_admin_page');
}

/**
 * Theme the output for the main flag administration page.
 */
function theme_flag_lists_admin_page() {
  drupal_set_title(t('Flag lists'));
  $output = '<p>' . t('This page shows all the <em>lists</em> that are currently defined on this system.') . '</p>';

  // Can we use our default view?
  if (module_exists('views')) {
    $view = views_get_view('flag_lists', FALSE);
    if (!empty($view)) {
      $view->set_display('default');
      $output .= $view->render();
    }
  }

  // Else build a simplified display.
  else {
    $header = array(
      array('data' => t('List'), 'field' => 'name'),
      array('data' => t('List title'), 'field' => 'title'),
      array('data' => t('List type')),
      array('data' => t('Owner'), 'field' => 'uid'),
      array('data' => t('Node types')),
      array('data' => t('Operations')),
    );
    $flags = flag_lists_get_flags(25, $header);
    foreach ($flags as $flag) {
      $ops = theme('flag_lists_ops', $flag);

      $name = db_result(db_query("SELECT name from {users} WHERE uid = %d", $flag->uid));
      $owner = l($name, 'user/'. $flag->uid, array('attributes' => array('title' => t('View user profile.'))));
      $rows[] = array(
        $flag->name,
        $flag->title,
        $flag->content_type,
        $owner,
        $flag->types ? implode(', ', $flag->types) : '-',
        $ops,
      );
    }
    if (!$flags) {
      $rows[] = array(
        array('data' => t('No flags are currently defined.'), 'colspan' => 6),
      );
    }

    $output .= theme('table', $header, $rows);
    $output .= theme('pager', NULL, 25, 0);
  }

  return $output;
}

function flag_lists_add($type = NULL) {
  if (is_null($type)) {
    drupal_access_denied();
  }

  return drupal_get_form('flag_lists_form', NULL, $type);
}

/**
 * Form to Add or edit a list.
 */
function flag_lists_form(&$form_state, $name = NULL, $type = NULL) {
  // First some sanity checks. $name and $type can't both be NULL.
  // There must be a template for this content type.
  if (is_null($name) && is_null($type)) {
    drupal_access_denied();
  }
  if (!flag_lists_template_exists($type) && is_null($name)) {
    return;
  }
  // If name is numeric, then we have the fid, so get the name.
  if (is_numeric($name)) {
    $name = db_result(db_query("SELECT name from {flag_lists_flags} WHERE fid = %s", $name));
  }
  // Adding a new list.
  if (!isset($name)) {
    drupal_set_title(t('Add a new list'));
    $form['edit'] = array(
      '#type' => 'hidden',
      '#value' => FALSE,
    );
  }

  // Editing an existing list.
  else {
    $flag = flag_lists_get_flag($name);
    drupal_set_title(t('Edit your "@title" @name title', array('@title' => $flag->get_title(), '@name' => variable_get('flag_lists_name', t('list')))));
    $form['edit'] = array(
      '#type' => 'hidden',
      '#value' => TRUE,
    );
    $form['name'] = array(
      '#type' => 'hidden',
      '#value' => $name,
    );
  }
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $flag->title,
    '#description' => t('A short, descriptive title for this @name list. Limit to 255 characters.', array('@name' => variable_get('flag_lists_name', t('list')))),
    '#maxlength' => 255,
    '#required' => TRUE,
    '#access' => empty($flag->locked['title']),
    '#weight' => -2,
  );
  $form['type'] = array(
    '#type' => 'hidden',
    '#value' => $type,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

/**
 * Validate a list.
 */
function flag_lists_form_validate($form, $form_state) {
  // Ensure 255 charactor or less name.
  if (drupal_strlen($form_state['values']['title']) > 255) {
    form_set_error('name', t('The @name title may only be 32 characters long.', array('@name' => variable_get('flag_lists_name', t('list')))));
  }
  // Ensure the machine name is unique.
  if (!$form_state['values']['edit']) {
    if (flag_lists_title_exists($form_state['values']['title'], $form_state['values']['type'])) {
      form_set_error('title', t('You already have a @name with this name for this type of content.', array('@name' => variable_get('flag_lists_name', t('list')))));
    }
  }
}

/**
 * Save a list.
 */
function flag_lists_form_submit($form, $form_state, $account = NULL) {
  if (!isset($account)) {
    $account = $GLOBALS['user'];
  }

  if ($form_state['values']['edit']) {
    // Editing the title.
    $flag = flag_lists_get_flag($form_state['values']['name']);
    $flag->title = $form_state['values']['title'];
    flag_lists_set_messages($flag);
    flag_lists_save($flag);
    _flag_lists_clear_cache();
  }
  else {
    // New flag. Load the template row.
    $type = $form_state['values']['type'];
    $row = db_fetch_object(db_query("SELECT * from {flags} f LEFT JOIN {flag_lists_types} fl on f.name = fl.name WHERE fl.type = '%s'", $type));

    $newflag = flag_flag::factory_by_content_type('node');

    $flag = $newflag->factory_by_row($row);
    // The template fid becomes the flag_lists parent flag.
    $flag->pfid = $row->fid;
    // The fid is NULL because this is really a new flag.
    $flag->fid = NULL;
    // The name is created in the save function.
    $flag->name = NULL;
    $flag->link_type = 'toggle';
    $flag->title = $form_state['values']['title'];;
    $flag->types = array($type);
    $flag->uid = $account->uid;
    flag_lists_set_messages($flag);
    // Save it.
    flag_lists_save($flag, $account);
  }
}

/**
 * Flag lists settings page.
 */
function flag_lists_settings_form(&$form_state) {
  drupal_set_title('Flag lists settings');
  $form['text'] = array(
    '#title' => t('Using flag lists'),
    '#value' => t('Flag lists allow users to create their own personal flags.
      No user can add to another user\'s lists. Lists inherit their
      settings from template flags, which exist as flags in the flags\' module.'
    ),
  );

  $form['flag_lists_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Substitute "list" with your own terminology'),
    '#default_value' => variable_get('flag_lists_name', t('list')),
    '#description' => t('You can choose to use another name for "list", such as "favorites" or "bookmarks". Lowercase and plural names usually work best.'),
    '#required' => TRUE,
  );
  $form['rebuild'] = array(
    '#type' => 'fieldset',
    '#title' => t('Global rebuild'),
    '#description' => t('Changes to list templates and  settings normally apply
      to only newly created flags. However, you can globally apply changes here.
      First adjust the settings above and the list templates and save them. Then
      click the link below. This will change ALL existing flag lists. It cannot
      be undone.'),
    '#tree' => FALSE,
  );
  $form['rebuild']['global_rebuild'] = array(
    '#value' => l('Rebuild all flag lists.', 'admin/build/flag_lists/rebuild'),
  );

  return system_settings_form($form);
}

/**
 * Confirm global settings rebuild.
 */
function flag_lists_rebuild_confirm($form_state) {
  return confirm_form($form,
    t('Are you sure you want to rebuild all flag lists?'),
    'admin/build/flags/lists/settings',
    t('This action cannot be undone.'),
    t('Rebuild'), t('Cancel')
  );
}

/**
 * Confirm global settings rebuild.
 */
function flag_lists_rebuild_confirm_submit($form, $form_state) {
  flag_lists_rebuild();
  drupal_set_message(t('All flag lists have been rebuilt.'));
  drupal_goto('admin/build/flags/lists/settings');
}

/**
 * Delete flag lists page.
 */
function flag_lists_delete_confirm(&$form_state, $name) {
  $flag = flag_lists_get_flag($name);
  if (empty($flag)) {
    drupal_goto();
  }

  $form['fid'] = array('#type' => 'value', '#value' => $flag->fid);

  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $flag->get_title())),
    isset($_GET['destination']) ? $_GET['destination'] : '/',
    t('This action cannot be undone.'),
    t('Delete'), t('Cancel')
  );
}

function flag_lists_delete_confirm_submit($form, &$form_state) {
  $flag = flag_lists_get_flag($form_state['values']['fid']);
  if ($form_state['values']['confirm']) {
    flag_lists_fl_delete($flag);
  }
}

/**
 * Form to create a new template.
 */
function flag_lists_create_template_form(&$form_state) {
  drupal_set_title(t('Add a new list template'));
  $form['help'] = array(
    '#value' => t('This form creates a new, blank list template. After saving, you will be able to configure further options.'),
  );
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Template name'),
    '#description' => t('The machine-name for this template. It may be up to 32 characters long and my only contain lowercase letters, underscores, and numbers.'),
    '#maxlength' => 255,
    '#required' => TRUE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

/**
 * New template validation.
 */
function flag_lists_create_template_form_validate($form, &$form_state) {
  // Ensure a safe machine name.
  if (!preg_match('/^[a-z_][a-z0-9_]*$/', $form_state['values']['name'])) {
    form_set_error('name', t('The flag name may only contain lowercase letters, underscores, and numbers.'));
  }
  // Ensure 32 charactor or less name.
  if (drupal_strlen($form_state['values']['name']) > 32) {
    form_set_error('name', t('The flag name may only be 32 characters long.'));
  }
  // Ensure the machine name is unique.
  $flag = flag_get_flag('fl_template_'. $form_state['values']['name']);
  if ($flag) {
    form_set_error('name', t('Flag names must be unique. This flag name is already in use.'));
  }
}

/**
 * New template submit.
 */
function flag_lists_create_template_form_submit($form, &$form_state) {
  $template = flag_lists_flag_default_flags('fl_template_'. $form_state['values']['name']);
  $flag = flag_flag::factory_by_array($template[0]);
  $flag->title = t('List template '). $form_state['values']['name'];
  $flag->save();
  // Enter the new template into flag_lists_types.
  db_query("INSERT INTO {flag_lists_types} (name) VALUES ('%s')", $flag->name);
  $form_state['redirect'] = 'admin/build/flags/edit/'. $flag->name;
}



/**
 * Developer tool to generate dummy lists and listings.
 */

// @todo use batch api.
function flag_lists_generate_lists_form() {
  $templates = flag_lists_get_templates();
  $options = array();
  foreach ($templates as $template) {
    $options[$template->fid] = $template->name . ' ('. implode(', ', $template->types) .')';
  }
  $form['templates'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Generate lists and/or listings for the following templates.'),
    '#options' => $options,
  );

  $form['lists'] = array(
    '#type' => 'textfield',
    '#title' => t('How many lists should be generated?'),
    '#default_value' => '0',
    '#size' => 7,
    '#maxlength' => 7,
  );

  $form['listings'] = array(
    '#type' => 'textfield',
    '#title' => t('How many listings should be generated?'),
    '#default_value' => '0',
  );

  $form['kill_lists'] = array(
    '#type' => 'checkbox',
    '#title' => t('Delete ALL existing lists and listings before generating new ones? Templates will not be deleted.'),
    '#default_value' => FALSE,
  );
  $form['kill_listings'] = array(
    '#type' => 'checkbox',
    '#title' => t('Delete ALL existing listings before generating new ones? (Lists are not deleted unless the above is checked.)'),
    '#default_value' => FALSE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Do it!'),
  );
  return $form;
}

/**
 * Validation handler for flag_lists_generate_lists_form.
 */
function flag_lists_generate_lists_form_validate($form, &$form_state) {
  if ($form_state['values']['lists'] && !is_numeric($form_state['values']['lists']) || $form_state['values']['lists'] < 0) {
    form_set_error('lists', t('Number of lists to generate must be a number, 1 or more.'));
  }
  if ($form_state['values']['listings'] && !is_numeric($form_state['values']['listings']) || $form_state['values']['listings'] < 0) {
    form_set_error('listings', t('Number of listings to generate must be a number, 1 or more.'));
  }
  if ($form_state['values']['lists'] > 0 && !count(array_filter($form_state['values']['templates']))) {
    form_set_error('templates', t('You must select at least 1 template for the lists to be genereated'));
  }
  if ($form_state['values']['listings'] > 0 && !count(array_filter($form_state['values']['templates']))) {
    form_set_error('templates', t('You must select at least 1 template for the listings to be genereated'));
  }
}

/**
 * Submit handler for flag_lists_generate_lists_form.
 */
function flag_lists_generate_lists_form_submit($form, &$form_state) {
  global $user;
  module_load_include('inc', 'devel_generate');

  // Delete listings.
  if ($form_state['values']['kill_listings']) {
      $flags = flag_lists_get_flags();
    foreach ($flags as $flag) {
      $result = db_query("SELECT fcid, content_id from {flag_lists_content} WHERE fid = %d", $flag->fid);
      while ($row = db_fetch_array($result)) {
        db_query("DELETE FROM {flag_lists_content} WHERE fid = %d", $flag->fid);
        db_query("DELETE FROM {flag_lists_counts} WHERE fid = %d", $flag->fid);
        module_invoke_all('flag', 'unflag', $flag, $row['content_id'], $account, $row['fcid']);
      }
    }
    drupal_set_message(t('All listings were deleted.'));
  }

  // Delete lists and listings.
  if ($form_state['values']['kill_lists']) {
    // If we loaded all flags above, don't reload them here.
    if (!count($flags)) {
      $flags = flag_lists_get_flags();
    }
    foreach ($flags as $flag) {
      flag_lists_fl_delete($flag, $user);
    }
    drupal_set_message(t('All lists and their listings were deleted.'));
  }


  $templates = array_filter($form_state['values']['templates']);
  $uids = array_filter(devel_get_users()); // Don't use the anon user.

  // Generate lists.
  if ($form_state['values']['lists'] && count($templates)) {
    for ($i = 1; $i <= $form_state['values']['lists']; $i++ ) {
      $template = flag_get_flag(NULL, array_rand($templates));
      $account->uid = $uids[array_rand($uids)];
      $edit['values']['title'] = devel_create_greeking(7, TRUE);
      $edit['values']['type'] = $template->types[array_rand($template->types)];
      $form = array();
      flag_lists_form_submit($form, $edit, $account);
    }
  drupal_set_message(t('@lists created.', array('@lists' => format_plural($form_state['values']['lists'], '1 list', '@count lists'))));
  }

  // Generate listings.
  if ($form_state['values']['listings']) {

    $count = 0;

    for ($i = 1; $i <= $form_state['values']['listings']; $i++ ) {
      $account->uid = $uids[array_rand($uids)];
      $lists = flag_lists_get_user_flags(NULL, $account);

      // Remove any lists that don't use the chosen templates.
      foreach ($lists as $key => $list) {
       if (!isset($templates[$list->pfid])) {
        unset($lists[$key]);
        }
      }

      // Ensure user has lists.
      if (!count($lists)) {
        continue;
      }

      $list = $lists[array_rand($lists)];
      $content_type = $list->types[array_rand($list->types)];

      // We get the random nid with 2 SELECTS to avoid slow ORDER BY Rand().
      // Get max nid for our content type.
      $max_nid = db_result(db_query("SELECT MAX(nid) as max FROM {node} WHERE type = '%s'", $content_type));

      // Ensure a node exists for the type.
      if (!$max_nid) {
        continue;
      }

      $r = rand(1, $max_nid);
      $content_id = db_result(db_query("SELECT nid from {node} WHERE type = '%s' AND nid >= %d LIMIT 1", $content_type, $r));

      // We can't assume that every attempt will result in a listing because
      // we may have a node type with no nodes yet or a user with no lists.
      // Also, if a listing already exists, we count it, but it can only be in
      // the db once.
      if (flag_lists_do_flag($list, 'flag', $content_id, $account, TRUE)){
        $count++;
      }
    }
  drupal_set_message(t('@listings listed.', array('@listings' => format_plural($count, '1 node', '@count nodes'))));
  if ($count < $form_state['values']['listings']) {
    drupal_set_message(t('Listings are generated randomly. Occassionally a listing will not be created if the random user has no lists or if the node type to be listed has no nodes.'));
  }
  }
}
