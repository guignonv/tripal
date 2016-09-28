<?php

class chado_feature__seqlen extends TripalField {
  // The default lable for this field.
  public static $default_label = 'Sequence length';

  // The default description for this field.
  public static $default_description = 'A field for calculating the length of a sequence.';

  // Add any default settings elements.  If you override the globalSettingsForm()
  // or the instanceSettingsForm() functions then you need to be sure that
  // any settings you want those functions to manage are listed in this
  // array.
  public static $default_settings = array(
    'chado_table' => '',
    'chado_column' => '',
    'base_table' => '',
    'semantic_web' => '',
  );

  // Set this to the name of the storage backend that by default will support
  // this field.
  public static $default_storage = 'field_chado_storage';

   /**
   * @see TripalField::formatterView()
   */
  public function formatterView(&$element, $entity_type, $entity, $langcode, $items, $display) {

    foreach ($items as $delta => $item) {
      $element[$delta] = array(
        '#type' => 'markup',
        '#markup' => $item['value'],
      );
    }
  }

  /**
   * @see TripalField::widgetForm()
   */
  public function widgetForm(&$widget, &$form, &$form_state, $langcode, $items, $delta, $element) {

    $settings = $this->field['settings'];
    $field_name = $this->field['field_name'];
    $field_type = $this->field['type'];
    $field_table = $this->field['settings']['chado_table'];
    $field_column = $this->field['settings']['chado_column'];

    $widget['value'] = array(
      '#type' => 'value',
      '#value' => array_key_exists($delta, $items) ? $items[$delta]['value'] : '',
    );

    $widget['feature__seqlen'] =  array(
      '#type' => 'value',
      '#value' => 0,
      '#title' => $element['#title'],
      '#description' => $element['#description'],
      '#weight' => isset($element['#weight']) ? $element['#weight'] : 0,
      '#delta' => $delta,
//      '#element_validate' => array('chado_feature__seqlen_widget_validate'),
    );
  }

}

/**
 * Callback function for validating the chado_feature__seqlen_widget.
 */
function chado_feature__seqlen_widget_validate($element, &$form_state) {
  $field_name = $element['#parents'][0];

  // Get the residues so we can calculate teh length.
  //$residues = tripal_chado_get_field_form_values('feature__residues', $form_state, 0, 'feature__residues');
  // Remove any white spaces.
  if ($residues) {
    $residues = preg_replace('/\s/', '', $residues);
    tripal_chado_set_field_form_values($field_name, $form_state, strlen($residues), 0, 'feature__seqlen');
  }
  else {
    // Otherwise, remove the seqlen value
    tripal_chado_set_field_form_values($field_name, $form_state, '__NULL__', 0, 'feature_seqlen');
  }
}