<?php
/**
 * @file index.php asmblr instance boot file.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */

/**
 * The path to asmblr's loader.
 *
 * Recommended paths for asmblr:
 *  - c:/inetpub/asmblr/
 *  - /var/www/asmblr/
 */
require('../core/load.inc');


if( empty($_GET['ok']))
{
    header('Retry-After: 3600');
    exit;
}

header('X-Powered-By: nunya');


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

        

);



// /**
//  * Begin application match and execution.
//  */
// $Request = \asm\Request::Init();
// $RequestHostname = \asm\Hostname::ToString($Request['Hostname']);


// if( isset($InstanceApps[$RequestHostname]) )
// {
//     $InstanceApps[$RequestHostname]['Hostname'] = $RequestHostname;
//     $App = $InstanceApps[$RequestHostname];

//     // hand control to our app's loader which is expected to carry on
//     include($App['AppRoot'].'/Load.inc');
// }
// else
// {
//     if( !empty($_SERVER['argc']) )
//         _stdo("CLI request domain not found");
//     else
//         \asm\HTTP::_404();

//     exit;
// }

