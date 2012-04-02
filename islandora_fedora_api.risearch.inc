<?php
/**
 * DO NOT USE, UNSTABLE
 * @file
 *   A group of functions meant to make interacting with the resource index easier
 * @author
 *   William Panting
 * This is deffinatly exparimental quality right now.
 * @todo support export as object
 * @todo support export as json serialized
 * @todo support export as array (paul did this for itql see lower in the file)
 * @todo examine use cases regarding objects in a deleted state: should they be returned on default queries?
 */


/**
 * This function will run any ri query.
 * @param string $query
 * @param array $arguments
 * @return
 * @todo error handling
 * @todo test if all frogs are green
 * @todo refactor this to use an array input rather than this long arg list
 */
function query_resource_index($query, $limit = -1, $offset = 0, $format='Sparql', $lang='sparql', $type='tuples', $flush='TRUE') {

  module_load_include('connection.inc', 'islandora_fedora_api');
  $fedora_connection = new FedoraConnection();

  $query_url = $fedora_connection->requestURL();

  //run query
  $query_url .= "/risearch?type=$type&flush=$flush" . (($limit > 0)?("&limit=$limit"):("")) . "&format=$format&offset=$offset&lang=$lang&query=" . urlencode($query);
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');

  $query_return_object = drupal_http_request($query_url);

  return $query_return_object->data;
}

/**
* This function is used to get a list containing all of the islandora collections in a Fedora repo
* @return $collection_list
*   an associated array of collection pids and names
*/
function get_all_collections() {

  $collection_list=array();

  //read in the itql query for getting collections
  $query_file_name=drupal_get_path('module', 'islandora_fedora_api') . '/collection_query.txt';
  $query_file_handle=fopen($query_file_name, "r");
  $query_string=fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);
  //make query
  $collection_list = get_related_objects($query_string);
  //strip out non-applicable collections via namespace
  $collection_list = limit_collections_by_namespace($collection_list);
  return $collection_list;
}

/**
* This function will reduce the results on a collection search down to those
* applicable to this install of Islandora.
* @author
*   Paul Pound
* @author
*   William Panting
* @param array $existing_collections
*   The list of collections before modification
* @param array $pid_namespaces
*   The list of namespaces that are applicable to this Islandora install
* @return array $collections
*   The collections that exist in the indicated namespaces
*/
function limit_collections_by_namespace($existing_collections, $pid_namespaces=null){
  //if no namespace list supplied get it from fedora_repository module's varaiables
  if ($pid_namespaces==null) {
    $pid_namespaces = array();
    foreach (explode(' ', trim(variable_get('fedora_pids_allowed', 'default: demo: changeme: Islandora: ilives: ')))as $namespace) {
      $pid_namespaces[$namespace] = $namespace;
    }
  }

  $collections = array();
  foreach($existing_collections as $collection => $value){
    foreach($pid_namespaces as $key => $namespace){
      if(strpos($collection,$namespace)===0){
        $collections[$collection]=$value;
      }
    }
  }
  return $collections;
}


/**
 *This function executes a query on Fedora's resource index
 * @param string $itql_query
 *   A query to use for searching the index
 * @return array $list_of_objects
 *   a nice array of objects
 */
function get_related_objects($itql_query) {

  module_load_include('inc', 'fedora_repository', 'CollectionClass');

  $collection_class = new CollectionClass();
  $query_results = $collection_class->getRelatedItems(NULL, $itql_query);
  $list_of_objects = itql_to_array($query_results);
  return $list_of_objects;
}

/**
 *This function turns an itql result into a usefull array
 * @author
 *   Paul Pound
 * @author
 *   William Panting
 * @param string $query_results
 *   The ugly string version
 * @return array $list_of_objects
 *   The well formed array version
 */
function itql_to_array($query_results) {
  try {
    $xml = new SimpleXMLElement($query_results);
  } catch (Exception $e) {
    drupal_set_message(t('Error getting list of collection objects !e', array('!e' => $e->getMessage())), 'error');
    return;
  }
  $list_of_objects = array();
  foreach ($xml->results->result as $result) {
    $a = $result->object->attributes();
    $temp = $a['uri'];
    $object = substr($temp, strrpos($temp, '/') + 1);
    $title = $result->title;
    $list_of_objects[$object] = (string) $title; //.' '.$object;
  }
  return $list_of_objects;
}

/**
 * This function gets all the members of a collection through the relationship of 'isMemberOf'
 * and 'isMemberOfCollection' the two relationships need to be checked because there is no
 * Fedora enforced standard.
 * @author
 *   William Panting
 * @param array $collection_id
 *   The collection to get the members of.
 * @return array $member_list_full
 *   The array containing all the pids of members of the collection.
 */
function get_all_members_of_collection($collection_id) {

  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');

  $member_list_full=array();

  $member_list1=get_related_items_as_array($collection_id, 'isMemberOf', 10000 , 0, FALSE);
  $member_list2=get_related_items_as_array($collection_id, 'isMemberOfCollection', 10000 , 0, FALSE);

  $member_list_full=array_merge($member_list1, $member_list2);

  return $member_list_full;
}

/**
 * This function is used to get a list containing all of the islandora collections in a Fedora repo
 * @author
 *   William Panting
 * @return $collection_list
 *   an associated array of collection pids and names
 * @todo make respect namespace restrictions
 * @todo remove workflow specific cruft
 */
/*
function get_all_collections() {

  $query_file_name = drupal_get_path('module', 'islandora_workflow') . '/collection_query.sparql';
  $query_file_handle = fopen($query_file_name, "r");
  $query_string = fread($query_file_handle, filesize($query_file_name));
  fclose($query_file_handle);

  $query_return_string = query_resource_index($query_string);

  if ($query_return_string) {
    $query_return_dom = DOMDocument::loadXML($query_return_string);
  }

  //format query results for use (xml->array)
  $collection_data = array();
  $result_nodes = $query_return_dom->getElementsByTagName('result');
  $result_nodes_length = $result_nodes->length;
  if ($result_nodes_length > 0) {
    for ($i=0; $i < $result_nodes_length; $i++) {
      $result_node = $result_nodes->item($i);

      $label_element = $result_node->getElementsByTagName('label')->item(0);
      $collection_pid_element = $result_node->getElementsByTagName('collection_object')->item(0);

      $collection_object_uri = $collection_pid_element->getAttribute('uri');
      //info:fedora/ is 12 chars
      $collection_object_pid = substr($collection_object_uri, 12, strlen($collection_object_uri));//need to extract the pid

      $collection_data[$collection_object_pid] = array();

      if ($label_element) {
        $label = $label_element->nodeValue;
        if ($label) {
          //the label may not be pupulated
          $collection_data[$collection_object_pid]['label'] = $label;
        }
      }
    }
  }
}*/