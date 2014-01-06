<?php
/**
 * @file Load.inc asmblr loader and global helper functions.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */


/**
 * asmblr root directory.
 */
define('ASM_ROOT',__DIR__.DIRECTORY_SEPARATOR);


/**
 * Shorthand for Log::Log().
 *
 * @see Log::Log()
 */
function llog( $Msg,$Level = 'LOG',$Backtrace = NULL,$Context = NULL )
{
    \asm\Log::Log($Msg,$Level,$Backtrace,$Context);
}



/**
 * Require the asmblr source code.
 */
require(ASM_ROOT.'Base.inc');
require(ASM_ROOT.'Request.inc');

require(ASM_ROOT.'Instance.inc');
require(ASM_ROOT.'App.inc');

require(ASM_ROOT.'Page.inc');
require(ASM_ROOT.'Template.inc');
require(ASM_ROOT.'Link.inc');

require(ASM_ROOT.'cnvyr.inc');

require(ASM_ROOT.'Misc.inc');

// Load the default en-US locale
require(ASM_ROOT.'enUS.inc');

