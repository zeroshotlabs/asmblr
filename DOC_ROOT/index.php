<?php
/**
 * @file index.php asmblr instance boot file.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */


// Optional
define('START_TIME',microtime(TRUE));

/**
 * The path to asmblr's loader.
 *
 * CONFIG: You should use an absolute path in production.
 *
 * Recommended paths for asmblr:
 *  - c:/inetpub/asmblr/core/Load.inc
 *  - /var/www/asmblr/core/Load.inc
 */
require('../core/Load.inc');


/**
 * Define known hostnames and their apps.
 *
 * Always use forward slashes in the directory.
 *
 * A Load.inc file is assumed to be in AppRoot to load and execute the application.
 *
 * The hostname will be used as the base hostname which can be overridden by BaseURL
 * when forming URLs.
 *
 * The pageset-name will be the default routing pageset for requests to that domain.  The
 * default routing pageset and it's linker will be available as ps and lp, respectively.
 *
 * Exact hostname matching is performed first, then ordered matching is performed,
 * least specific to most specific, which match domains prefixed with a period.
 */
$InstanceApps = array
(
    'mc.centz'=>array('AppRoot'=>'/var/www/asmblr-apps/MC',
                    'RoutingPS'=>'mc',
                'CacheManifest'=>FALSE,
                     'CacheApp'=>FALSE),

    'mcm.centz'=>array('AppRoot'=>'/var/www/asmblr-apps/MC',
                     'RoutingPS'=>'mcm',
                 'CacheManifest'=>FALSE,
                      'CacheApp'=>FALSE),

    'swcom.centz'=>array('AppRoot'=>'/var/www/asmblr-apps/www.stackware.com',
                       'RoutingPS'=>'sw',
                   'CacheManifest'=>FALSE,
                        'CacheApp'=>FALSE)
);



/**
 * Begin application match and execution.
 */
$Request = \asm\Request::Init();
$RequestHostname = \asm\Hostname::ToString($Request['Hostname']);

if( isset($InstanceApps[$RequestHostname]) )
{
    $InstanceApps[$RequestHostname]['Hostname'] = $RequestHostname;
    $App = $InstanceApps[$RequestHostname];

    // hand control to our app's loader which is expected to carry on
    include($App['AppRoot'].'/Load.inc');
}

