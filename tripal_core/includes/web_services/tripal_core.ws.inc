<?php

require_once 'tripal_core.ws_chado.inc';
require_once 'tripal_core.ws_cv.inc';

/**
 * Uses the query string to determine which web services object to create.
 */
function tripal_core_ws() {
  global $base_url;

  // Determine which web services to provide.
  $query_string = $base_url . $_SERVER['REQUEST_URI'];
  $matches = array();
  $ws = NULL;

  $re_base_url = preg_replace('/\//', '\/', $base_url);
  if (preg_match("/^$re_base_url\/ws\/v(\d+\.\d+)\/(\w+)/", $query_string, $matches)) {
    $version = $matches[1];
    $ws_type = $matches[2];
    switch ($ws_type) {
      case 'chado':
         $ws = new ChadoWS();
        break;
      default:
        $ws = new VocabWS();
    }
  }

  if ($ws) {
    $ws->processQuery();
  }
}

/**
 *
 */
class TripalWebServices {

  private $version;
  private $status;
  private $message;
  private $links;
  private $embedded;

  /**
   *
   */
  public function __construct() {
    $this->result = array();
    $this->status = 'success';
    $this->version = 'v1.0';
    $this->message = '';
    $this->links = array();
    $this->embedded = array();
  }
  /**
   *
   */
  public function __destruct() {

  }
  /**
   *
   * @param unknown $limit
   */
  public function setPagerLimit($limit) {
    $page_limit = $limit;
  }
  /**
   *
   * @param unknown $id
   */
  public function setPagerId($id) {
    $pager_id = $id;
  }
  /**
   *
   * @return string
   */
  public function getVersion() {
    return $this->version;
  }
  /**
   *
   * @return string
   */
  public function getBaseURL() {
    global $base_url;
    return $base_url . '/ws/' . $this->getVersion();
  }
  /**
   *
   */
  public function processQuery() {
  }

  /**
   *
   */
  public function sendResponse() {
    global $base_url;

    $response['status']  = $this->status;
    $response['message'] = $this->message;
    $response['version'] = $this->version;
    $response['source'] = array(
      'site_name' => variable_get('site_name', "Unspecified"),
      'site_url' => $base_url,
      'site_slogan' => variable_get('site_slogan', "Unspecified"),
      'site_email' =>  variable_get('site_mail', "Unspecified"),
    );
    $response['result']  = $result;
    print drupal_json_output($response);
  }
}