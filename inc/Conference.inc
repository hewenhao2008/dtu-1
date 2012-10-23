<?php

class Conference {
  protected $fedora_item;
  protected $pid;
  
  public function __construct($fedora_item) {
    $this->fedora_item = $fedora_item;
    $this->pid = $fedora_item->pid;
  }
  
  public function create() {
    //$this->fedora_item->add_datastream_from_file(drupal_get_path('module', 'fedora_repository') . '/images/Crystal_Clear_filesystem_folder_grey.png', 'TN', 'Thumbnail.png', 'image/png', 'M');
    $this->addContributionCollection($this->pid);
    $this->addDocumentCollection($this->pid);
    $this->updateMXE();
  }
  
  public function update() {
    $mxe = $this->fedora_item->get_datastream_dissemination('MXE');
    $mxe_xml = new SimpleXMLElement($mxe);
    // update Contributions' <super_events>
    $contributions = dtu_get_contributions($this->pid);
    foreach ($contributions AS $contribution) {
      $this->updateContribution($contribution['pid'], $mxe_xml);
    }

    $modified = dtu_mxe_tidy(&$mxe_xml);
    if ($modified) {
      $this->fedora_item->modify_datastream_by_value($mxe_xml->asXML(), 'MXE', 'MXE', 'text/xml');
    }
    // update documents <event>
  }
  
  private function addContributionCollection($parent_pid) {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    
    $contributions = Fedora_Item::ingest_new_item($pid = '', $state = 'A', $label = 'Contributions', $owner = '');
    $contributions->add_relationship('hasModel', 'islandora:collectionCModel', FEDORA_MODEL_URI);
    $contributions->add_relationship('isMemberOfCollection', $parent_pid);
    $contributions->add_relationship('contributionCollection', $parent_pid);
    $collection_policy = <<<EOD
    <collection_policy xmlns="http://www.islandora.ca" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" name="" xsi:schemaLocation="http://www.islandora.ca http://syn.lib.umanitoba.ca/collection_policy.xsd">
      <content_models>
        <content_model dsid="ISLANDORACM" name="Islandora Collection Model ~ islandora:collectionCModel" namespace="islandora:1" pid="islandora:collectionCModel"></content_model>
        <content_model dsid="ISLANDORACM" name="Contribution CModel" namespace="default:1" pid="dtu:contributionCModel"></content_model>
      </content_models>
      <search_terms></search_terms>
      <staging_area></staging_area>
      <relationship>isMemberOfCollection</relationship>
    </collection_policy>
EOD;
    
    $contributions->add_datastream_from_string($collection_policy, 'COLLECTION_POLICY', 'COLLECTION_POLICY', 'text/xml', 'X');
    $contributions->add_datastream_from_file(drupal_get_path('module', 'fedora_repository') . '/images/Crystal_Clear_filesystem_folder_grey.png', 'TN', 'Thumbnail.png', 'image/png', 'M');
  }
  
  private function addDocumentCollection($parent_pid) {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  
    $documents = Fedora_Item::ingest_new_item($pid = '', $state = 'A', $label = 'Documents', $owner = '');
    $documents->add_relationship('hasModel', 'islandora:collectionCModel', FEDORA_MODEL_URI);
    $documents->add_relationship('isMemberOfCollection', $parent_pid);
    $documents->add_relationship('documentCollection', $parent_pid);
    $document_collection_policy = <<<EOD
    <collection_policy xmlns="http://www.islandora.ca" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" name="" xsi:schemaLocation="http://www.islandora.ca http://syn.lib.umanitoba.ca/collection_policy.xsd">
      <content_models>
        <content_model dsid="ISLANDORACM" name="Islandora Collection Model ~ islandora:collectionCModel" namespace="islandora:1" pid="islandora:collectionCModel"></content_model>
        <content_model dsid="ISLANDORACM" name="Document CModel" namespace="default:1" pid="dtu:documentCModel"></content_model>
      </content_models>
      <search_terms></search_terms>
      <staging_area></staging_area>
      <relationship>isMemberOfCollection</relationship>
    </collection_policy>
EOD;

    $documents->add_datastream_from_string($document_collection_policy, 'COLLECTION_POLICY', 'COLLECTION_POLICY', 'text/xml', 'X');
    $documents->add_datastream_from_file(drupal_get_path('module', 'fedora_repository') . '/images/Crystal_Clear_filesystem_folder_grey.png', 'TN', 'Thumbnail.png', 'image/png', 'M');
  }
  
  private function updateContribution($cont_pid, $conf_mxe_xml) {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    
    $contribution = new Fedora_Item($cont_pid);
    $cont_mxe = $contribution->get_datastream_dissemination('MXE');

    $cont_mxe_xml = new SimpleXMLElement($cont_mxe);
    
    $super_event = $cont_mxe_xml->xpath('//super_event[@event_type="conference"]');
    $super_event[0]->title->main = (string) $conf_mxe_xml->title->original->main;
    $super_event[0]->title->short = (string) $conf_mxe_xml->title->original->short;
    
    if (!$super_event[0]->organisation) {
      $super_event[0]->addChild('organisation');
    }
    $super_event[0]->organisation = (string) $conf_mxe_xml->location->venue;
    if (!$super_event[0]->uri) {
      $super_event[0]->addChild('uri');
    }
    $super_event[0]->uri = (string) $conf_mxe_xml->uri;
    $contribution->modify_datastream_by_value($cont_mxe_xml->asXML(), 'MXE', 'MXE', 'text/xml');

    $document_nodes = $cont_mxe_xml->xpath('//documents/document');

    $documents = array();
    foreach ($document_nodes AS $document_node) {
      $documents[] = $document_node->attributes()->id;
    }

    foreach ($documents AS $document) {
      $this->updateDocument($document, $conf_mxe_xml);
    }
  }
  
  private function updateDocument($doc_pid, $conf_mxe_xml) {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    
    $document = new Fedora_Item($doc_pid);
    $doc_ddf = $document->get_datastream_dissemination('DDF');

    $doc_ddf_xml = new SimpleXMLElement($doc_ddf);
    $conference_event = $doc_ddf_xml->xpath('//event/id[text()="' . $this->pid . '"]/..');

    $conference_event[0]->title->full = (string) $conf_mxe_xml->title->original->main;
    $conference_event[0]->title->acronym = (string) $conf_mxe_xml->title->original->short;
    $conference_event[0]->title->number = (string) $conf_mxe_xml->title->number;

    $conference_event[0]->dates->start = (string) $conf_mxe_xml->date->start;
    $conference_event[0]->dates->end = (string) $conf_mxe_xml->date->end;

    $place = '';
    if (!empty($conf_mxe_xml->location->city)) {
      $place .= (string) $conf_mxe_xml->location->city;
    }
    if (!empty($conf_mxe_xml->location->country)) {
      $place .= ', ' . (string) $conf_mxe_xml->location->country;
    }

    if (!$conference_event[0]->place) {
      $conference_event[0]->addChild('place');
    }
    $conference_event[0]->place = $place;

    if (!$conference_event[0]->uri) {
      $conference_event[0]->addChild('uri');
    }
    $conference_event[0]->uri = (string) $conf_mxe_xml->uri;

    $document->modify_datastream_by_value($doc_ddf_xml->asXML(), 'DDF', 'DDF', 'text/xml');
  }

  /**
   * Finalize the MXE document with elements and attributes that are difficult to add via the form builder workflow.
   */
  function updateMXE() {
    $mxe = $this->fedora_item->get_datastream_dissemination('MXE');
    
    $mxe_xml = new SimpleXMLElement($mxe);

    $mxe_xml->addAttribute('format_version', '0.1.1');
    $mxe_xml->addAttribute('eve_lang', 'eng');
    $mxe_xml->addAttribute('eve_type', 'eco');
    $mxe_xml->addAttribute('eve_year', (string) $mxe_xml->date->year);
    $mxe_xml->addAttribute('rec_id', $this->pid);
    $mxe_xml->addAttribute('rec_source', 'dtu_conferences');
    $mxe_xml->addAttribute('rec_status', 'c');

    $mxe_xml->addChild('id', $this->pid);

    dtu_mxe_tidy(&$mxe_xml);
    
    $this->fedora_item->modify_datastream_by_value($mxe_xml->asXML(), 'MXE', 'MXE', 'text/xml');
  }
}