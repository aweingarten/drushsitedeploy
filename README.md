Drush Site Deploy
==============


Overview
---------

Drush Site Deploy provides a single Drush command to deploy a Drupal site.
It reads deployment settings from a yaml file that you customize.

In this case the word deployment means taking code from version control, deploying
it to a server, and then executing commands and updates against the db server.

When we deploy updates to a Drupal site we want to see reproducible results. A
smooth, quiet deployment, which achieves a clear outcome, is always the goal.
It gets developers into bed at a somewhat reasonable hour and keeps PMs and
stakeholders happy.

There are other existing solutions for deploying a build. The usual suspects are:
- manual deployment
- scripts
- CI Server like Jenkins
- Aegir

For some these are great solutions.  However, they often require knowledge outside Drupal.
This approach is an inherently drupal solution.  That allows people to leverage their
drupal knowledge for deployment.


Dependencies
------------

  - Drush 6.x or higher.
  - Drupal 7.  (D6 and D8 are not yet supported).
  - Composer recommended for installing third party libraries.  Composer will install
    symphony2/yaml and guzzle.


Usage
-----
### Installation
1: You can copy this project to any of the following locations on the target machine.
  - .drush folder in your HOME folder.
  - folder sym-linked to the .drush folder in your HOME folder.
  - /usr/share/drush/commands (configurable)
  - In an arbitrary folder specified with the --include option.

  (Note this must be installed on the machine you are referencing with a drush alias. NOT locally.)

2: Install symphony2 and guzzle using composer:

   `composer install`

   don't have composer?
    - https://getcomposer.org/download/


3: Verify sitedeploy is installed:
   `drush sitedeploy --example`


### Quick start
You can copy the sample deployment yaml file from /api/sample_deployment.yaml.
Alternatively you can run the following command to get the most up-to-date
listing:
    `drush sitedeploy --buildoptions`

Customize the yaml file as necessary to meet your needs.  We recommend you save it somewhere
under source control.

The target environment is interpolated from the site that drush bootstraps.
If you use drush aliases the alias will specify the target environment.

Now you are ready to go:
    `drush sitedeploy <buildfile> <vcs-tag>`

Kickback and enjoy the show!

### Developers
 Implement the following hooks to extend SiteDeploy with your own Drush
 projects (see working examples for many of these in Acquia Deploy). They will
 alter $config object and allow you to execute any deployment steps specific to
 your process.

   - sitedeploy_setup ($config),
   - sitedeploy_codedeploy($config)
   - sitedeploy_postcodedeploy ($config)
   - sitedeploy_cleanup ($config)


#### Working with YAML
For many people YAML is new and unfamiliar.  For more info checkout:
    - http://www.yaml.org/start.html
    - http://symfony.com/doc/current/components/yaml/yaml_format.html



Theory
---------
When a deployment is complete we want to ensure that a site is in a known
idempotent state. This is a really important and powerful point. We should be
able to follow the deployment steps over and over again and achieve the same
expected results. This is how we can judge the suitability of a deployment
solution.

However, we often fall short of that goal. In reality many Drupal deployments
are often complicated with lots of manual steps.

#### What’s wrong with manual deployment steps?

Any time that you employ manual deployment steps you introduce an unstable,
unpredictable and ultimately unreliable element, yourself. It’s not that I don’t
trust you. It’s that I don’t trust anyone (myself included to get it right).
The more manual steps the more opportunities there are to screw it up.


#### Build VS Deployment
I would make a distinction between building and deploying.  A
site build in the context drupal deals with pulling code together.  Recommend
checking out: [buildmanager](https://github.com/WhiteHouse/buildmanager) or [drushsubtree](https://github.com/WhiteHouse/drushsubtree)

Those tools are concerned with pulling code from multiple sources. Site
deployment starts where those build tools stop. It takes a git tag and
goes through the steps to get live and kicking on a web-server.


#### Drupal’s traditional built-in deployment tools
Out of the box drupal provides one tool for site deployment: Update Hooks.

Update hooks are functions that are called on update.php/drush updb on a per-
module basis. Each update hook has a version number associated with it. E.g.
- mymodule_update_7001
- mymodule_update_7002
- etc

Each updates runs once and only once. To ensure this the version number for
the update is stored in the DB System table. If you fire up you can see exactly
what version some of your favorite modules are on. Update hooks are on a per-
module basis so different modules can be on different version numbers. They
are a great way to eliminate manual deployment steps. Anything that you can do
with a manual deployment step can also be done in an update hook.


#### Update Hooks: What's the worst that could happen?
The typical “Drupal Way” for deployment would involve a deployment using an
install profile to stand up a site. Then as the site is developed and grown there
would be update hooks written each release. Every release you would run drush
updb and all your deployments steps would automatically execute. Sounds
great? Problems solved!

What happens though if despite all your best efforts a deployment fails release
QA and you are forced to roll back to your previous release. Depending on what
was contained in your update hooks you might be able to rollback the code
without having to restore a DB backup. However, a few weeks later if you tried to
re-deploy you would find that none of your update hooks would run. Why? They
already ran once. They are not designed to be run again. In order to make them
run again you would need to go into the DB update the schema version for each
module to ensure that it would run again.

Another problem is what happens if the desired results of an update hook are
undone by a rogue users, or module? How would you ensure that your site was
in the correct state? There would be no way for you to re-run the deployment to
ensure your site is in the expected idempotent state. This is a HUGE problem. If
you don’t know the state of your site with any degree of certainty then the site is
effectively out of control.

#### I already have a continuous integration tools to do this?
Contiuous integration tools like Jenkins are awesome but they aren't the right
fir

#### I can write shell scripts to invoke drush and do the same thing?
Often shell scripts are inefficient and their writers may not take the time to
fully understand the complexity of drush.  For example "drush pm-enable"
enables a module and then does a cache clear all.  Cache clear takes a while.
Imagine doing one for each module that's enabled.


FAQs:
----------

- Q: Why aren't my hook implementations triggering?
- A: Assuming that the hook are correct try running `drush cache-clear drush`

- Q: Drush is telling me that "drushsitedeploy" cannot be found.
- A: First check and see if site deploy is properly installed. Refer to the installation Section.
     Also make sure that drushsitedeploy is installed on the machine you are targeting with drush.  If
     you are using a drush alias this is probably a remote machine NOT your local.

- Q: How do I create a drush hook to extend drushsitedeploy?




TO DO
---------
- Return correct error codes along with exceptions.  These are used for drush
  exit codes.

- clean up output around the webservice calls.  When they are thinking.  Explain the ....
  wrap them to 80 chars.

- make tool unit testable.

- implement helper text.

- implement command to display sample file.  Have hooks so that other extensions
  can add to sample file.

- By default drupal hooks in a drush extension will not execute unless the
  extension is registered as a drupal module.  Drush commands do a lot of
  additional work to work around this limitation. To collect as much information
  as possible I would like to implement hook_watchdog and print to the screen.

  Investigating writing directly to the DB during bootstrap to register drupal
  hooks in the extension.  This could be a universally helpful item.  Once drush
  exists the DB hacks would need to be removed.

  Thinking this can be a new Drush extension.  Seems like this has universal
  applicability.

    - https://api.drupal.org/api/drupal/includes%21module.inc/function/module_implements/7
    - cache_bootstrap -> module_implements   or  static $drupal_static_fast in module.inc

 - Come up a solution for how to handle D6/D7/D8 with the same branch.


#### longer term todo
- D6 Testing
- rollback?
- notifications
- deployment plan

-log anything anomalous
    - modules that are active but not included in the files
    - features that are active but not included in the files
    - fields, content types, taxonomies, permissions, variables which are not serialized in the module




