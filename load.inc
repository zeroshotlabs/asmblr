<?php declare(strict_types=1);
/**
 * @file load.inc asmblr loader.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */


/**
 * asmblr root and extension directories.
 */
define('ASM_ROOT',__DIR__.DIRECTORY_SEPARATOR);
define('ASM_EXT_ROOT',__DIR__.DIRECTORY_SEPARATOR.'extensions'.DIRECTORY_SEPARATOR);


/**
 * Current working/execution root directory; not the APP_ROOT.
 */
define('CWD_ROOT',getcwd().DIRECTORY_SEPARATOR);




/**
 * Require the asmblr source code.
 */
require(ASM_ROOT.'core/exception.php');
require(ASM_ROOT.'core/types.php');
require(ASM_ROOT.'core/sys.php');

require(ASM_ROOT.'core/HTTP.php');

require(ASM_ROOT.'core/request.php');

require(ASM_ROOT.'core/config.php');
require(ASM_ROOT.'core/app.php');

// require(ASM_ROOT.'App.php');
// require(ASM_ROOT.'AppT.php');

// require(ASM_ROOT.'Page.php');
require(ASM_ROOT.'core/endpoint.php');
require(ASM_ROOT.'core/template.php');
require(ASM_ROOT.'core/Link.php');

require(ASM_ROOT.'core/Misc.php');

// Load the default en-US locale
// @todo update according to FAIQ - probably require promptd
require(ASM_ROOT.'core/enUS.php');
