<?php
/**
 * @file Load.inc asmblr loader and global helper functions.
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
define('ASM_EXT_ROOT',ASM_ROOT.'ext'.DIRECTORY_SEPARATOR);

/**
 * Current working/execution root directory; not the APP_ROOT.
 */
define('CWD_ROOT',getcwd().DIRECTORY_SEPARATOR);




/**
 * Require the asmblr source code.
 */
require(ASM_ROOT.'exception.php');
require(ASM_ROOT.'types.php');
require(ASM_ROOT.'debug.php');

require(ASM_ROOT.'HTTP.php');

require(ASM_ROOT.'request.php');

require(ASM_ROOT.'config.php');
require(ASM_ROOT.'asmd.php');

// require(ASM_ROOT.'App.php');
// require(ASM_ROOT.'AppT.php');

// require(ASM_ROOT.'Page.php');
require(ASM_ROOT.'endpoint.php');
require(ASM_ROOT.'template.php');
require(ASM_ROOT.'Link.php');

require(ASM_ROOT.'Misc.php');

// Load the default en-US locale
// @todo update according to FAIQ - probably require promptd
require(ASM_ROOT.'enUS.php');

