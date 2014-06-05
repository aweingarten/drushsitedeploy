<?php
/**
 * @file
 * Acquia Cloud API Operations
 *
 * Contains functions to interact
 */

/*
 * States: done, waiting, error, received
 * might make sense to create an abstract class with constants to store these
 * values.
 *
 * Instead of printing here might make sense to throw exceptions and provide
 * more central error/exception handling.
 */


/**
 * Implements hook_sitedeploy_predeploy().
 */
function acquiadeploy_sitedeploy_setup($config_file) {
  if (!isset($_ENV['AH_SITE_GROUP'])) {
    throw new Exception("Your environment seems to be missing expected acquia site variable AH_SITE_GROUP you may be running against the wrong environment.");
  }

  // https://docs.acquia.com/cloud/configure/env-variable#animated
  if ($_ENV['AH_PRODUCTION']) {
    $deploy_to_prod = drush_confirm("You are about to deploy to PROD are you sure you REALLY want to do this?", 3);

    if (!$deploy_to_prod) {
      exit(0);
    }
  }
  // @TODO make sure that there are no other current tasks on the server.

  $acquia_deploy = new AcquiaDeploy(
    $_ENV['AH_SITE_GROUP'],
    $_ENV['AH_SITE_ENVIRONMENT'],
    $config_file[acquia_cloudapi_username],
    $config_file[acquia_cloudapi_password],
    $config_file[acquia_ignore_cert_mismatch],
    $config_file[acquia_cloudapi_url]);


  $acquia_backup_db = isset($config_file[acquia_backup_db]) ? $config_file[acquia_backup_db] : 0;
  if ($acquia_backup_db) {
    $acquia_deploy->backupDatabase();
  }
}


/**
 * Implements hook_sitedeploy_deploy().
 */
function acquiadeploy_sitedeploy_codedeploy($config_file) {
  // https://docs.acquia.com/cloud/configure/env-variable#animated
  if ($_ENV['AH_PRODUCTION']) {
    DeployPrinter::printMessage("You are about to deploy to production are you sure you really want to do this?");
  }

  // @TODO make sure that there are no other current tasks on the server.
  $target_environment = $_ENV['AH_SITE_ENVIRONMENT'];

  $acquia_deploy = new AcquiaDeploy(
                                $_ENV['AH_SITE_GROUP'],
                                $target_environment,
                                $config_file[acquia_cloudapi_username],
                                $config_file[acquia_cloudapi_password],
                                $config_file[acquia_ignore_cert_mismatch],
                                $config_file[acquia_cloudapi_url]);

  $acquia_copy_db = isset($config_file[acquia_copy_db]) ? $config_file[acquia_copy_db] : 0;
  if ($acquia_copy_db) {
    $acquia_deploy->copyDatabase($config_file[acquia_copy_db_src], $target_environment);
  }

  $acquia_copy_files = isset($config_file[acquia_copy_files]) ? $config_file[acquia_copy_files] : 0;
  if ($acquia_copy_files) {
    $acquia_deploy->copyFiles($config_file[acquia_copy_files_src], $target_environment);
  }

  // @todo might want to make this optional.
  $acquia_deploy->deployTag($config_file[vcs_tag]);
}


/**
 * Implements hook_sitedeploy_cleanup().
 */
function drush_sitedeploy_cleanup($config_file) {
  if (!isset($_ENV['AH_SITE_GROUP'])) {
    throw new Exception("Your environment seems to be missing expected acquia site variable AH_SITE_GROUP you may be running against the wrong environment.");
  }

  $acquia_deploy = new AcquiaDeploy(
    $_ENV['AH_SITE_GROUP'],
    $_ENV['AH_SITE_ENVIRONMENT'],
    $config_file[acquia_cloudapi_username],
    $config_file[acquia_cloudapi_password],
    $config_file[acquia_ignore_cert_mismatch],
    $config_file[acquia_cloudapi_url]
  );

  if (isset($config_file[acquia_backup_db])) {
    $acquia_deploy->clearVarnish($config_file[acquia_site_domain]);
  }
}


class AcquiaDeploy {
  private $site;
  private $env;
  private $username;
  private $password;
  private $webserviceClient;
  private $cloudapiUrl;

  /**
   * Constructor for Acquia deploy class.
   *
   * @param string $site
   *   Name of the site.
   * @param string $env
   *   Name of the environment.
   * @param string $username
   *   Username credential.
   * @param string $password
   *   Password credential.
   * @param bool $ignore_cert_mismatch
   *   True/false value to allow or deny certificates mismatch.
   */
  public function __construct($site, $env, $username, $password, $ignore_cert_mismatch, $acquia_cloudapi_url) {
    $this->site = $site;
    $this->env = $env;
    $this->username = $username;
    $this->password = $password;
    $this->setupWebServiceClient($ignore_cert_mismatch);

    // @todo check for https: in the $acquia_cloudapi_url.  If missing add it.
    $this->cloudapiUrl = isset($acquia_cloudapi_url) ? $acquia_cloudapi_url : "https://cloudapi.acquia.com/";
  }

  /**
   * Sets up Guzzle client for the class.
   *
   * @param bool $ignore_cert_mismatch
   *   Bool telling the method to allow mismatched or invalid SSL certs to be
   *   accepted.
   */
  private function setupWebServiceClient($ignore_cert_mismatch) {
    // http://cloudapi.acquia.com/#POST__sites__site_envs__env_dbs__db_backups-instance_route
    //$this->webserviceClient = new GuzzleHttp\Client();
    $this->webserviceClient = new \Guzzle\Http\Client();
    if ($ignore_cert_mismatch) {
      $this->webserviceClient->setDefaultOption('verify', FALSE);
    }
    $this->webserviceClient->setDefaultOption('auth', array($this->username, $this->password));
  }

  /**
   * Constructor for Acquia deploy class.
   *
   * @param string $acquia_task_id
   *   Id of the current task to monitor.
   *
   * @return array
   *   Response object from webservice call.
   * @throws
   *  Message when webservice call fails.
   */
  private function getTaskStatus($acquia_task_id) {
    // Set up the webservice request.
    $service_params = array(":site" => $this->site, ":acquia_task_id" => $acquia_task_id);
    $service_query = $this->cloudapiUrl . "/v1/sites/:site/tasks/:acquia_task_id.json";
    $service_query = strtr($service_query, $service_params);

    // Just in case this call fails we can retry a few times.
    for ($i = 0; $i < 10; $i++) {
      print (".");
      $request = $this->webserviceClient->get($service_query);
      $response = $request->send();
      $success_pulling_webservice = $response->getStatusCode() == 200;
      if ($success_pulling_webservice) {
        return $response;
      }
    }
    throw new exception("Error: unable to pull-down the status update from the server..");
  }


  /**
   * Initiates a webservice call against Acquia's API.
   *
   * Most of the webservice calls take time to complete.  This method is
   * designed to check and block till the request is completed.  The method will
   * check the server every minute for status updates.
   *
   * @param array $service_params
   *   Arguments to be passed to the webservice.
   * @param string $service_query
   *   Query template to be sent to the webservice..
   * @param string $service_name
   *   Short name for logging and error messages.
   *
   * @throws
   *   Webservice failure message.
   */
  private function performCloudTask($service_params, $service_query, $service_name) {
    $service_query = strtr($service_query, $service_params);


    // Make webservice call.
    $request = $this->webserviceClient->post($service_query);
    $response = $request->send();
    DeployPrinter::printMessage("The acquia cloud api call $service_name has been started:");

    // Check if it there were problems.
    //print_r("Response" . $response);


    $problem_pulling_webservice = $response->getStatusCode() != 200;



    if ($problem_pulling_webservice) {

      throw new exception("Error: the $service_name failed with a response code of $response please try again.");
    }

    // Task has started.  Now we need to wait for it to complete.  Will check the task status once a minute.
    $response_json = $response->json();
    $acquia_task_id = $response_json[id];
    $backup_running = TRUE;

    DeployPrinter::printMessage("Checking the status.");
    while ($backup_running) {
      sleep(60);
      $response = $this->getTaskStatus($acquia_task_id);
      $response_json = $response->json();
      $backup_running = $response_json[state] == "started";
    }

    // Newline to ensure the next print is on a newline.
    drush_print("");

    $task_completed_successfully = $response_json[state] == "done" && isset($response_json[completed]);
    if (!$task_completed_successfully) {
      throw new exception("Error: the $service_name failed with a response code of $response please try again.");
    }

    DeployPrinter::printMessage("$service_name completed successfully.");
  }


  /**
   * Copy a Database from a source to a target environment.
   *
   * Returns when the DB copy is complete.
   *
   * @param string $source
   *   Source environment to copy from.
   * @param string $target
   *   Destination environment to copy to.
   */
  public function copyDatabase($source, $target) {
    // http://cloudapi.acquia.com/#POST__sites__site_dbs__db_db_copy__source__target-instance_route
    // Set up the webservice request.
    $service_params = array(
      ":site" => $this->site,
      ":db" => $this->site,
      ":source" => $source,
      ":target" => $target);

    $service_query = $this->cloudapiUrl .
                        "/v1/sites/:site/dbs/:db/db-copy/:source/:target.json";

    $this->performCloudTask($service_params, $service_query, "copyDatabase");
  }

  /**
   * Copy files from a source to a target environment.
   *
   * Returns when the file transfer is complete.
   *
   * @param string $source
   *   Source environment to copy from.
   * @param string $target
   *   Destination environment to copy to.
   */
  public function copyFiles($source, $target) {
    // http://cloudapi.acquia.com/#POST__sites__site_envs__env_code_deploy-instance_route
    $service_params = array(
      ":site" => $this->site,
      ":source" => $source,
      ":target" => $target);

    $service_query = $this->cloudapiUrl .
                            "/v1/sites/:site/files-copy/:source/:target.json";
    $this->performCloudTask($service_params, $service_query, "copyFiles");
  }

  /**
   * Backs up a database in Acquia cloud.  Returns once the backup is complete.
   */
  public function backupDatabase() {
    // Set up the webservice request.
    $service_params = array(":site" => $this->site, ":env" => $this->env);
    $service_query = $this->cloudapiUrl .
                          "/v1/sites/:site/envs/:env/dbs/:site/backups.json";

    $this->performCloudTask($service_params, $service_query, "backupDatabase");
  }

  /**
   * Deploys tag in Acquia Cloud.
   *
   * Returns when the deployment is complete.
   *
   * @param string $tag
   *   The GIT tag to deploy to the current environment.
   */
  public function deployTag($tag) {
    // http://cloudapi.acquia.com/#POST__sites__site_envs__env_code_deploy-instance_route
    // Set up the webservice request.
    $service_params = array(
      ":site" => $this->site,
      ":env" => $this->env,
      ":tag" => $tag);

    // @todo include path combine here.
    $service_query = $this->cloudapiUrl . "/v1/sites/:site/envs/:env/code-deploy.json?path=tags%2F:tag";


    $this->performCloudTask($service_params, $service_query, "deployTag");
  }

  /**
   * Clears varnish cache for domain.
   *
   * Returns when the backup is complete.
   *
   * @param string $domain
   *   The domain to clear varnish on.
   */
  public function clearVarnish($domain) {
    // http://cloudapi.acquia.com/#DELETE__sites__site_envs__env_domains__domain_cache-instance_route
    $service_params = array(
      ":site" => $this->site,
      ":env" => $this->env,
      ":domain" => $domain);
    $service_query = $this->cloudapiUrl . "/v1/sites/:site/envs/:env/domains/:domain/cache.json";
    $this->performCloudTask($service_params, $service_query, "clearVarnish");
  }
}
