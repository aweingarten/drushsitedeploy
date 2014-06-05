Acquia Site Deploy
==============

Overview
---------

Acquia deploy is a plugin for DrushSiteDeploy which provides functions for
interacting with Acquia Cloud.  It makes use of Acquia's cloud API.

Acquia deploy implements hooks from DrushSiteDeploy and looks for specific
config settings in the DrushSiteDeploy yaml file.  See below for details.


Dependencies
------------
  - drushsitedeploy

  - Drush 6.x or higher.

  - Composer recommended for installing third party libraries.  It will install
    symphony2 and guzzle.



Usage
-----
### Installation
1: You can copy this project to any of the following:
  - .drush folder in your HOME folder.
  - folder sym-linked to the .drush folder in your HOME folder.
  - /usr/share/drush/commands (configurable)
  - In an arbitrary folder specified with the --include option.

2: Install DrushSiteDeploy


###Config Options
There is no code :)  Add the following to your SiteDeployConfig file:

```
#####################################
# Acquia Cloud API Credentials
#####################################
acquia_cloud_api_username:
acquia_cloud_api_password:
acquia_ignore_cert_mismatch: 1
acquia_backup_db: 1
acquia_copy_db: 1
acquia_copy_db_src: prod
acquia_flush_varnish_cache: 1
acquia_copy_files: 1
acquia_copy_files_src: prod
acquia_site_domain: dev.mysite.net  #used for varnish
#Can I backup and restore acquia cron jobs for env  : nope :(
```

#### Working with YAML
For many people YAML is new and unfamiliar.  For more info checkout:
    - http://www.yaml.org/start.html
    - http://symfony.com/doc/current/components/yaml/yaml_format.html

