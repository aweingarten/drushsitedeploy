<?php
/**
 * @file
 * sitedeploy drush command.
 *
 * You can copy this project to any of the following:
 *    1. A .drush folder in your HOME folder.
 *    2. /usr/share/drush/commands (configurable)
 *    3. In an arbitrary folder specified with the --include option.
 */

require_once 'drushsitedeploy_steps.inc';
require_once 'vendor/autoload.php';


/**
 * Implements hook_drush_command().
 */
function sitedeploy_drush_command() {
  $items = array();
  $items['sitedeploy'] = _cmd_sitedeploy();
  $items['sitedeploy-diff'] = _cmd_sitedeploy_diff();
  $items['sitedeploy-sampleconfig'] = _cmd_sitedeploy_sampleconfig();
  return $items;
}


/**
 * Builds the drush command object array for sitedeploy.
 *
 * @return array
 *   containing the parameters for a single drush command.
 */
function _cmd_sitedeploy() {
  return array(
    'description' => "Deployment system for a new site.",
    'arguments' => array(
      'deployfile' => 'Deployment config file',
      'tag' => 'Git Tag to deploy to environment',
    ),
    'examples' => array(
      'drush sitedeploy --deployfile mydeployment.yaml --tag 7.x-2.2' => 'Deploys a site.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX,
  );
}

/**
 * Builds the drush command object array for sitedeploy.
 *
 * @return array
 *   containing the parameters for a single drush command.
 */
function _cmd_sitedeploy_sampleconfig() {
  return array(
    'description' => "Deployment system for a new site.",
    'arguments' => array(
      'deployfile' => 'Deployment config file',
      'tag' => 'Git Tag to deploy to environment',
    ),
    'examples' => array(
      'drush sitedeploy --deployfile mydeployment.yaml --tag 7.x-2.2' => 'Deploys a site.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX,
  );
}


/**
 * Drush command callback function. Entry point for the diff process.
 *
 * This is no currently implemented.  The vision here is to predict the changes
 * to a site before performing a deployment.  Can help detect unexpected changes
 * before it's too late.
 */
function drush_sitedeploy_diff() {
  throw new Exception("Not implemented. Will be in a later release.");
}

/**
 * The actual code deployment step.
 *
 * The goal of setup is to lock-down/prep the site and get it to a safe
 * point where we feel comfortable deploying.  Only minimally invasive steps
 * should be included here.
 */
function drush_sitedeploy_buildoptions() {
  DeployPrinter::printSection("Config File Sample");

  drush_command_invoke_all('sitedeploy_config_sample');
}

/**
 * Drush command callback function. Entry point for the sitedeploy process.
 */
function drush_sitedeploy() {

  try{
    DeployPrinter::printWelcome();

    $args = func_get_args();
    $deploy_filename = $args[0];
    $tag = $args[1];
    $config_file = sitedeploy_get_config($deploy_filename);
    $config_file[vcs_tag] = $tag;

    $sds = new SiteDeploySteps($config_file);
    $sds->run();
    DeployPrinter::printBuildCompletion();
  }

  catch (Exception $e) {
    drush_set_error("Exception was thrown: " . $e->getMessage());
    exit($e->getCode());
  }

  return 0;
}


/**
 * Load config from config yaml file.
 *
 * @param string $config_file_name
 *   Path to the config yaml file.
 *
 * @return array
 *   Return parsed YAML config as PHP array.
 *
 * @throws
 * exception containing error message.
 */
function sitedeploy_get_config($config_file_name) {

  if (!class_exists('Symfony\Component\Yaml\Parser')) {
    // For Drush PSR-0 and Composer information, see:
    // http://drupal.org/node/1316322.
    throw new Exception('Autoloading Symfony\Component\Yaml\Parser failed.');
  }

  if (!file_exists($config_file_name)) {
    throw new Exception("Error: Could not find the config file.  Please double check the path and try again.");
  }

  try{
    $parser = new Symfony\Component\Yaml\Parser();
    $config = $parser->parse(file_get_contents($config_file_name));
  }

  catch (Exception $e) {
    throw new Exception("Error: Hmm it looks like there was a problem loading the config.  Please check it for valid formatting. The error was" . $e->getMessage());
  }

  return $config;
}


/**
 * Builds the drush command object array for sitedeploy-diff.
 *
 * Site deploy diff is intended to run through a site deployment file and
 * compare it to a current site and determine the items that will change based
 * on the deployment.  This should give you an idea of the severity of the
 * deployment before running
 *
 * @return array
 *   containing the parameters for a single drush command.
 */
function _cmd_sitedeploy_diff() {
  return array(
    'description' => "Shows a deployment plan.",
    'options' => array(
      'deployfile' => 'Deployment config file',
      'tag' => 'Git Tag to deploy to environment',
    ),
    'examples' => array(
      'drush sitedeploy --deployfile mydeployment.yaml --tag 7.x-2.2' => 'Deploys a site.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
}

class SiteDeployNotifier {
  /**
   * Sends a notification to the notification receipients.
   *
   * Need to research if there is an existing framework we can easily leverage.
   * Would be helpful for tasks that might take a while say a DB backup/restore.
   */
  public static function sendNotification() {
    // Have this fire a hook and allow people to plugin what they want.
    // Can have SMS, email, and IRC as supported modules.
  }
}


/**
 * Implements hook_watchdog().
 */
function sitedeploy_watchdog($log_entry) {
  // @todo make it work!
  drush_print("\nhoot2!\n$log_entry");
}


class DeployPrinter {

  /**
   * Prints welcome message for the tool.
   */
  public static function printWelcome() {
    drush_print("\n " . str_repeat('=', 79));
    drush_print(" Drush Site Deploy");
    drush_print(" " . str_repeat('=', 80) . "\n");
  }

  /**
   * Prints build section message with a subheader format.
   *
   * @param string $message
   *   Message to display to user.
   */
  public static function printSection($message) {
    drush_print(" \n " . $message);
    drush_print(" " . str_repeat('-', 79));
  }

  /**
   * Prints build message. Includes indentation and timestamp on each row.
   *
   * @param string $message
   *   Message to display to user.
   */
  public static function printMessage($message) {
    $time = date("H:i:s", time());
    drush_print("   " . $time . ": " . $message);
  }

  /**
   * Prints build completion message.
   */
  public static function printBuildCompletion() {
    self::printMessage("Build completed successfully");
    drush_print("\n\n");
  }
}
