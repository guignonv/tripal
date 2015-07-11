<?php

class ChadoWS extends TripalWebServices {

  /**
   *
   */
  public function __construct() {
    parent::__construct();
  }
  /**
   * @see TripalWebServices::__destruct()
   */
  public function __destruct() {
    parent::__destruct();
  }

 /**
  * Overrides TripalWebServices::getBaseURL()
  *
  * Sets the base URL of the web services to includ the '/chado' suffix.
  *
  * @see TripalWebServices::getBaseURL()
  */
  public function getBaseURL() {
    global $base_url;
    $chado_url = $base_url . '/ws/' . $this->getVersion() . '/chado';
    return $chado_url;
  }
  /**
   *
   */
  public function processQuery() {
    global $base_url;
    $query_string = $base_url . $_SERVER['REQUEST_URI'];
    $query_args = drupal_get_query_parameters();

    // Convert the Chado WS URL for use in regular expressions.
    $chado_url = trim($this->getBaseURL());
    $re_chado_url = preg_replace('/\//', '\/', $chado_url);
    $re_chado_url = preg_replace('/\./', '\.', $re_chado_url);

    // URL for the table listing.
    $matches = array();
    $result = array();
    if (preg_match("/^$re_chado_url$/", $query_string)) {
      $result = $this->getAllTables();
    }
    // URL for a listing of table records.
    elseif (preg_match("/^$re_chado_url\/(\w+)$/", $query_string, $matches)) {
      $table_name = $matches[1];
      $result = $this->getAllTableRecords($table_name);
    }
    // URL for the table schema.
    elseif (preg_match("/^$re_chado_url\/(\w+)\/schema$/", $query_string, $matches)) {
      $table_name = $matches[1];
      $result = $this->getTable($table_name);
    }
    // URL for a record.
    elseif (preg_match("/^$re_chado_url\/(\w+)\/(\d+)/", $query_string, $matches)) {
      $table_name = $matches[1];
      $id = $matches[2];
      $result = $this->getTableRecord($table_name, $id, $query_args);
    }
    else {
      $result = 'Unknown command';
    }

    // The responses follow the same format as the AGAVE API with a
    // status, message, version and all data in the "result" object.
    $response['status']  = $query_string;
    $response['message'] = $re_chado_url;
    $response['version'] = $this->getVersion();
    $response['source'] = array(
      'site_name' => variable_get('site_name', "Unspecified"),
      'site_url' => $base_url,
      'site_slogan' => variable_get('site_slogan', "Unspecified"),
      'site_email' =>  variable_get('site_mail', "Unspecified"),
    );
    $response['result']  = $result;
    print drupal_json_output($response);
  }

  /**
   *
   */
  private function getAllTables() {
    $api_url = $this->getBaseURL();
    $tables = chado_get_table_names(FALSE);
    foreach ($tables as $table) {
      $result['_links']['chado_tables'][] = array('href' => "$api_url/$table");
    }
    return $result;
  }
  /**
   *
   * @param unknown $table_name
   * @throws Exception
   */
  private function getTable($table_name) {
    if (!chado_table_exists($table_name)) {
      throw new Exception("Table, '$table_name', is not a valid table.");
    }
    return chado_get_schema($table_name);
  }
  /**
   *
   * @param unknown $table_name
   * @throws Exception
   */
  private function getTableSchema($table_name) {
    if (!chado_table_exists($table_name)) {
      throw new Exception("Table, '$table_name', is not a valid table.");
    }
    return chado_get_schema($table_name);
  }
  /**
   *
   * @param $table_name
   * @param $id
   * @param $query_args
   */
  private function getTableRecord($table_name, $id, $query_args = array()){
    if (!chado_table_exists($table_name)) {
      throw new Exception("Table, '$table_name', is not a valid table.");
    }
    // Get the primary key name for this table.
    $schema = chado_get_schema($table_name);
    $pkey = $schema['primary key'][0];

    // Get the record.
    $values = array($pkey => $id);
    $options = array('return_array' => 1);
    $var = chado_generate_var($table_name, $values, $options);
    $item = $var[0];

    // Do any expansion requested.
    if (count($query_args) > 0) {
      $item = $this->expandObject($item, $query_args);
    }

    // Recursively reformat the expanded objects to match HAL requirements.
    return $this->formatObject($table_name, $item, $schema, $query_args);
  }

  /**
   *
   * @param unknown $table_name
   * @param unknown $query_args
   * @return Ambigous <multitype:string number unknown , unknown>
   */
  private function getAllTableRecords($table_name, $query_args = array()) {
    $api_url = $this->getBaseURL();
    $schema = chado_get_schema($table_name);
    $pkey = $schema['primary key'][0];
    $pager_id = 0;
    $page_limit = 25;
    $result = array();

    // Get the record.
    $values = array();
    $options = array(
      'return_array' => 1,
      'pager' => array(
        'limit' => $page_limit,
        'element' => $pager_id
      ),
    );
    $var = chado_generate_var($table_name, $values, $options);

    // Get the total number of records
    $total = chado_pager_get_count($pager_id);
    $curr_page = array_key_exists('page', $query_args) ? $query_args['page'] : 0;

    $first_page = '0';
    $last_page = ceil($total / $page_limit) - 1;
    $result['_links']['first'] = array('href' => "$api_url/$table_name");
    if ($curr_page > 0) {
      $prev_page = $curr_page - 1;
      if ($prev_page != $first_page) {
        $result['_links']['previous'] = array('href' => "$api_url/$table_name?page=$prev_page");
      }
      else {
        $result['_links']['previous'] = $result['_links']['first'];
      }
    }
    if ($curr_page < $last_page) {
      $next_page = $curr_page + 1;
      $result['_links']['next'] = array('href' => "$api_url/$table_name?page=$next_page");
    }
    if ($last_page > $first_page) {
      $result['_links']['last'] = array('href' => "$api_url/$table_name?page=$last_page");
    }

    // Add the number of elements for this collection
    $result['count'] = count($var);
    $result['total'] = (integer) $total;
    $result['current_page'] = (integer) $curr_page;
    $result['items_per_page'] = $page_limit;

    // Do any expansion requested.
    if (count($query_args) > 0) {
      $var = $this->expandObject($var, $query_args);
    }

    // recursively reformat the expanded objects to match HAL requirements.
    foreach ($var as $item) {
      $item = $this->formatObject($table_name, $item, $schema, $query_args);
      $result['_embedded'][$table_name][] = $item;
    }
    return $result;
  }

  /**
   *
   * @param unknown $var
   * @param unknown $query_args
   * @throws Exception
   * @return A
   */
  private function expandObject($var, $query_args) {
    $api_url = $this->getBaseURL();

    $page_limit = 25;
    $pager_id = 0;
    $options = array(
      'return_array' => 1,
      'pager' => array(
        'limit' => $page_limit,
        'element' => $pager_id
      ),
    );

    // If the user has requested to expand any fields then do that
    if (array_key_exists('expand_table', $query_args)) {
      $expand_tables = explode(',', $query_args['expand_table']);
      foreach($expand_tables as $table) {
        // Does the table exist?
        if(!chado_table_exists($table)) {
          throw new Exception("Table, '$table', is not a valid table and thus cannot be expanded.");
        }

        // Expand the variable.
        $var = chado_expand_var($var, 'table', $table, $options);

        // if the table could not be expanded then the chado_expand_var
        // function just returns an empty record but sets the table name
        // in the object. For the JSON, we still want to create an _embedded
        // record so we have to create a stdClass object and set the
        // table name.
        if (property_exists($var, $table) and !isset($var->$table)) {
          $var->$table = new stdClass();
          $var->$table->tablename = $table;
        }
      }
    }
    if (array_key_exists('expand_field', $query_args)) {
      $expand_fields = explode(',', $query_args['expand_field']);
      foreach($expand_fields as $field) {
        // TODO: check to make sure the field exists
        $var = chado_expand_var($var, 'field', $field);
      }
    }
    if (array_key_exists('expand_fkey', $query_args)) {
      $expand_fkeys = explode(',', $query_args['expand_fkey']);
      foreach($expand_fkeys as $fkey) {
        // TODO: check to make sure the fkey exists
        $var = chado_expand_var($var, 'foreign_key', $fkey);
      }
    }

    return $var;
  }
  /**
   *
   * @param $table_name
   * @param $object
   * @param $schema
   * @param $query_args
   * @throws Exception
   * @return unknown
   */
  private function formatObject($table_name, $object, $schema, $query_args = array()) {

    global $base_url;

    $api_url = $this->getBaseURL();
    $pkey = $schema['primary key'][0];
    $id = $object->$pkey;

    // Add the self link first
    if ($id) {
      $object->_links['self'] = array('href' => "$api_url/$table_name/$id");
      $object->_links['show_expansion'] = array('href' => "$api_url/$table_name/$id?show_expansion=1");
    }
    else {
      $object->_links['self'] = array('href' => "$api_url/$table_name");
      $object->_links['show_expansion'] = array('href' => "$api_url/$table_name?show_expansion=1");
    }

    // Add the links for the table.
    $object->_links["tables"] = array('href' => "$api_url");
    $object->_links["schema"] = array('href' => "$api_url/$table_name/schema");

    // Add the link to the Drupal page if a node exists.
    if (property_exists($object, 'nid')) {
      $object->_links["view"] = array('href' => $base_url . url("node/$object->nid"));
      // Unset the node ID because it's really only needed within the context
      // of the local Drupal site.
      unset($object->nid);
    }

    // It doesn't make sense to allow expansion of node information outside
    // of the context of the local Drupal site so remove this object.
    unset($object->expandable_nodes);

    // Only include links for expanding if the option to exclude them has not
    // been passed.
    if (!array_key_exists('show_expansion', $query_args)) {
      unset($object->expandable_fields);
      unset($object->expandable_foreign_keys);
      unset($object->expandable_tables);
    }

    // Deal with the expandable tables/fields/fkeys/nodes. Sometimes there are
    // specified in PHP with explicit numerical indexes and sometimes not. But,
    // the JSON converter will maintain the indexes if present which creates
    // an inconsistent look. So, we use the array_values function to just
    // get the list.
    if (array_key_exists('expandable_tables', $object)) {
      $object->expandable_tables = array_values($object->expandable_tables);
      if (count($object->expandable_tables) > 0) {
        $object->_links["expand_table"][] = array('href' => "$api_url/$table_name?expand_table={table}[,{table}...]");
        $object->_links["expand_table"][] = array('href' => "$api_url/$table_name/$id/?expand_table={table}[,{table}...]");
      }
      else {
        unset($object->expandable_tables);
      }
    }
    if (array_key_exists('expandable_fields', $object)) {
      $object->expandable_fields = array_values($object->expandable_fields);
      if (count($object->expandable_fields) > 0) {
        $object->_links["expand_field"][] = array('href' => "$api_url/$table_name?expand_field={field}[,{field}...]");
        $object->_links["expand_field"][] = array('href' => "$api_url/$table_name/$id?expand_field={field}[,{field}...]");
      }
      else {
        unset($object->expandable_fields);
      }
    }
    if (array_key_exists('expandable_foreign_keys', $object)) {
      $object->expandable_foreign_keys = array_values($object->expandable_foreign_keys);
      if (count($object->expandable_foreign_keys) > 0) {
        $object->_links["expand_fkey"][] = array('href' => "$api_url/$table_name?expand_fkey={fkey}[,{fkey}...]");
        $object->_links["expand_fkey"][] = array('href' => "$api_url/$table_name/$id?expand_fkey={fkey}[,{fkey}...]");
      }
      else {
        unset($object->expandable_foreign_keys);
      }
    }

    // iterate through the items in the object and see if they in turn are
    // objects.  If so, then recurse.
    foreach ($object as $key => $value) {

      // If any values are objects then recurse and format them correctly.
      if (is_object($value)) {

        $table_name = $value->tablename;
        $schema = chado_get_schema($table_name);
        if ($schema) {
          // Replace the object with the actual value if it exists.  If there is
          // no key value then this is probably an expanded table so just unset
          if (property_exists($value, $key)) {
            $object->$key = $value->$key;
          }
          else {
            unset($object->$key);
          }
          // Recursively format the object.
          $value = $this->formatObject($table_name, $value, $schema, $query_args);
          // Add the object as an "_embedded" object of the JSON.
          if (property_exists($object,'_embedded') and
            array_key_exists($table_name, $object->_embedded)) {
              // If the element is already an array then add to it, otherwise
              // convert it into an array.
              if (is_array($object->_embedded[$table_name])) {
                $object->_embedded[$table_name][] = $value;
              }
              else {
                $first = $object->_embedded[$table_name];
                $object->_embedded[$table_name] = array();
                $object->_embedded[$table_name] = $first;
                $object->_embedded[$table_name][] = $value;
              }
            }
            // This is the first time this embedded table has been seen
            // there fore, add the value as a single element.
            else {
              $object->_embedded[$table_name] = $value;
            }
        }
        else {
          throw new Exception("Table, '$table_name', is not a valid table.");
        }
      }
    }

    if (array_key_exists('no_links', $query_args)) {
      unset($object->_links);
    }

    return $object;
  }
}