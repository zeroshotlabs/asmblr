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
 * Shorthand for Log::Log().
 *
 * @see Log::Log()
 * @todo This could probably be improved according to named params.
 */
function llog( $Msg,$Level = 'LOG',$Backtrace = NULL,$Context = NULL )
{
    foreach( func_get_args() as $V )
        \asm\Log::Log($V,$Level,$Backtrace,$Context);
}


/**
 * Shorthand for getting the $app object.
 *
 * @note Do not overuse.
 */
function asm()
{
    return $GLOBALS['asmapp'];
}


/**
 * Require the asmblr source code.
 */
require(ASM_ROOT.'Base.php');
require(ASM_ROOT.'DAO.php');

require(ASM_ROOT.'Request.php');

require(ASM_ROOT.'App.php');
require(ASM_ROOT.'AppT.php');

require(ASM_ROOT.'Page.php');
require(ASM_ROOT.'Template.php');
require(ASM_ROOT.'Link.php');

require(ASM_ROOT.'restr.php');
require(ASM_ROOT.'cnvyr.php');

require(ASM_ROOT.'Misc.php');

// Load the default en-US locale
require(ASM_ROOT.'enUS.php');

