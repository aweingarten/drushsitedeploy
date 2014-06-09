<?php

/**
 * @file
 * Site Deployment plan for all the steps that need to happen as part of a
 * deployment.
 */


class SiteDeploySteps {
  private $configFile;

  /**
   * Constructor for SiteDeploySteps.
   *
   * @param array $config_file
   *   The config_file contains settings loaded from yaml config file.
   */
  public function __construct($config_file) {
    $this->configFile = $config_file;
  }

  /**
   * Kicks off the whole process.
   */
  public function run() {
    $this->setup();
    $this->disableModules();
    $this->codeDeploy();
    $this->postCodeDeploy();
    $this->enableModules();
    $this->enableThemes();
    $this->setVariables();
    $this->cleanUp();
  }

  /**
   * Prep the site for the deployment.
   *
   * The goal of setup is to lock-down/prep the site and get it to a safe
   * point where we feel comfortable deploying.  Only minimally invasive steps
   * should be included here.
   */
  private function setup() {
    DeployPrinter::printSection("Kicking off the deployment:");

    $has_hook_to_execute = count(drush_command_implements('sitedeploy_setup')) > 0;
    if ($has_hook_to_execute) {
      drush_command_invoke_all_ref('sitedeploy_setup', $this->configFile);
    }


    if (isset ($this->configFile[maintenance_mode])) {
      variable_set('maintenance_mode', $this->configFile[maintenance_mode]);
      DeployPrinter::printMessage("Putting Site into maintenance mode");

      // @todo make this a reusable function for setting variables.

      $maintenenance_mode = variable_get('maintenance_mode');
      if ($maintenenance_mode) {
        DeployPrinter::printMessage("Confirmed that site is in maintenance mode");
      }
    }

    if (isset ($this->configFile[maintenance_mode_message])) {
      variable_set('maintenance_mode_message', $this->configFile[maintenance_mode_message]);
      DeployPrinter::printMessage("Setting the maintenance mode message to: \"" . $this->configFile[maintenance_mode_message] . "\"");
    }
  }

  /**
   * The actual code deployment step.
   *
   * The goal of setup is to lock-down/prep the site and get it to a safe
   * point where we feel comfortable deploying.  Only minimally invasive steps
   * should be included here.
   */
  private function codeDeploy() {
    DeployPrinter::printSection("Deploying Code");

    $has_hook_to_execute = count(drush_command_implements('sitedeploy_codedeploy')) > 0;
    if ($has_hook_to_execute) {
      drush_command_invoke_all_ref('sitedeploy_codedeploy', $this->configFile);
    }
  }

  /**
   * Any steps that need to be completed post code-deployment.
   */
  private function postCodeDeploy() {
    DeployPrinter::printSection("Post Code Deployment Tasks");


    $has_hook_to_execute = count(drush_command_implements('sitedeploy_postcodedeploy')) > 0;
    if ($has_hook_to_execute) {
      drush_command_invoke_all_ref('sitedeploy_postcodedeploy', $this->configFile);
    }
  }

  /**
   * Invokes a post-deploy hook once all sitedeploy code is complete.
   */
  private function cleanUp() {
    DeployPrinter::printSection("Clean up tasks");
    DeployPrinter::printMessage("Taking site out of maintenance mode");
    variable_set('maintenance_mode', 0);

    $has_hook_to_execute = count(drush_command_implements('sitedeploy_cleanup')) > 0;
    if ($has_hook_to_execute) {
      drush_command_invoke_all_ref('sitedeploy_cleanup', $this->configFile);
    }
  }

  /**
   * Enables modules listed in the config array.
   */
  private function enableModules() {
    // Enable Modules.
    $installed_module_list = module_list();

    if (!isset($this->configFile[module_enable])) {
      return;
    }

    DeployPrinter::printMessage("Enabling Modules");
    foreach ($this->configFile[module_enable] as $key => $value) {

      $have_modules_to_enable = count($value) > 0;
      if ($have_modules_to_enable) {
        DeployPrinter::printMessage("The following: $key modules will be enabled:");

        // Only going to enable modules which aren't currently enabled.
        $modules_to_enable = array_diff($value, $installed_module_list);

        // @todo need to figure out how to hook into watchdog from drush.  Would provide meaningful feedback here.
        module_enable($modules_to_enable);
      }
    }
  }

  /**
   * Disables modules listed in the config array.
   */
  private function disableModules() {
    $module_disable = $this->configFile[module_enable];

    foreach ($module_disable as $key => $value) {
      $have_modules_to_disable = count($value) > 0;

      if ($have_modules_to_disable) {
        DeployPrinter::printMessage("The following: $key modules will be disabled:");
        module_disable($value);
      }
    }
  }

  /**
   * Enables themes listed in the config array.
   */
  private function enableThemes() {
    $has_themes = count($this->configFile[themes]) > 0;
    if ($has_themes) {
      DeployPrinter::printMessage("Enabling Themes: ");
      theme_enable($this->configFile[themes]);
    }
  }

  /**
   * Sets variable with confirmation to user.
   *
   * Does variable_get for confirmation.
   *
   * @param string $key
   *   The variable to set.
   *
   * @param string $value
   *   The value to set.
   */
  private function deploySetVariable($key, $value) {
    DeployPrinter::printMessage("Setting variable:$key to $value.   ");
    variable_set($key, $value);

    $stored_value = variable_get($key);
    if ($stored_value == $value) {
      DeployPrinter::printMessage("confirmed");
    }

    else {
      throw new Exception("Unable to set the variable: " . $key);
    }
  }


  /**
   * Grabs variables from the config object and sets them in the Drupal db.
   */
  private function setVariables() {
    $variables = $this->configFile[variables];
    $has_variables = count($variables) > 0;

    if ($has_variables) {
      DeployPrinter::printMessage("Setting Variables: ");
      foreach ($variables as $key => $value) {
        self::deploySetVariable($key, $value);
      }
    }
  }
}
