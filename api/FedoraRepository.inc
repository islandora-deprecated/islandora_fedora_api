<?php
/**
 * @file
 * This file contains the class FedoraRepository. It wraps the RAW Fedora REST functions to provide an
 * interface to repository functions that don't operate on objects, including instantiating an object.
 */

/**
 * Wrapper for Fedora Commons API Respository functions (functions that don't operate on existing objects).
 */
class FedoraRepository {

  /**
   * The FedoraConnection object for this FedoraRespository.
   *
   * @var FedoraConnection
   */
  public $connection;

  /**
   * Constructs the FedoraRepository object. Creates a new FedoraAPI object, and optionally if
   * one isn't supplied it creates a new FedoraConnection object.
   *
   * @param FedoraConnection $connection
   *   (optional) If this arguement isn't supplied a new FedoraConnection is created.
   */
  function __construct($connection = NULL) {
    if (empty($connection)) {
      $connection = new FedoraConnection();
    }

    $this->connection = $connection;
    $this->apia = new FedoraAPIA($this->connection);
    $this->apim = new FedoraAPIM($this->connection);
  }

  /**
   * Ingest a new object, with either an FOXML file, an FOXML string or neither for a blank object.
   *
   * @param string $foxml_file
   *   (optional) Used if ingesting a FOXML file.
   * @param string $foxml_string
   *   (optional) Used if ingesting FOXML from a string.
   * @param array $options
   *   (optional) An array of options to be passed to the ingest functions. They array keys for this array are:
   *   pid, new, label, format, encoding, namespace, ownerId, logmessage.
   *
   * @return FedoraItem the newly-created item.
   *
   * @throws FedoraAPIRestException
   */
  function ingest(
    $foxml_file = NULL, $foxml_string = NULL,
    $options = array(
      'pid'        => NULL,
      'new'        => NULL,
      'label'      => NULL,
      'format'     => NULL,
      'encoding'   => NULL,
      'namespace'  => NULL,
      'ownerId'    => NULL,
      'logMessage' => NULL,
    )
  ){

    $response = $this->apim->ingest(
      $foxml_file, $foxml_string,
      !empty($options['pid']) ? $options['pid'] : NULL,
      !empty($options['new']) ? $options['new'] : NULL,
      !empty($options['label']) ? $options['label'] : NULL,
      !empty($options['format']) ? $options['format'] : NULL,
      !empty($options['encoding']) ? $options['encoding'] : NULL,
      !empty($options['namespace']) ? $options['namespace'] : NULL,
      !empty($options['ownerId']) ? $options['ownerId'] : NULL,
      !empty($options['logMessage']) ? $options['logMessage'] : NULL
    );

    if ($response->code != 201) {
      throw new FedoraAPIRestException($response);
    }
    return new FedoraItem($response->data);
  }

  /**
   * Wrapper for Fedora's findObjects API function.
   * @param string $terms Search terms, e.g. "changeme:*"
   * @param string $query Detailed search query, e.g. "pid=demo:12"
   * @param int $max_results
   * @param array $display_fields 
   * @return array The result set with the specified fields populated
   */
  function findObjects($terms = NULL, $query = NULL, $max_results = 50, $display_fields = array('pid', 'label', 'state', 'ownerId', 'cDate', 'mDate', 'dcmDate',
    'title', 'creator', 'subject', 'description', 'publisher', 'contributor', 'date', 'type',
    'format', 'identifier', 'source', 'language', 'relation', 'coverage', 'rights')) {

    // pid needs to be part of the displayFields so we can construct the results array.  
    if (!in_array('pid', $display_fields)) {
      $display_fields[] = 'pid';
    }
    $resp = $this->apia->findObjects($terms, $query, $max_results, 'xml', $display_fields);
    if ($resp->code != 200) {
      throw new FedoraRestException($resp->error);
    }
    $results = new SimpleXMLElement($resp->data);
    $resultset = array();
    foreach ($results->resultList->objectFields as $obj) {
      $pid = (string) $obj->pid;
      $resultset[$pid] = array();

      foreach ($obj->children() as $field) {
        $field_name = $field->getName();
        if ($field_name != 'pid') {
          $resultset[$pid][$field_name] = (string) $field;
        }
      }
    }
    return $resultset;
  }

  /**
   * Simple wrapper for findObjects that returns TRUE if an object with the 
   * specified pid exists in the repository.
   * @param string $pid
   * @return boolean
   */
  public function objectExists($pid) {
    $result = $this->findObjects(NULL, "pid=$pid");
    return count($result) == 1;
  }

}