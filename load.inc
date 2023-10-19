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
 * 
 * @note Generally important for CLI execution.
 */
define('CWD_ROOT',getcwd().DIRECTORY_SEPARATOR);


/**
 * Require the asmblr source code - ordered.
 */
require(ASM_ROOT.'core/types.php');
require(ASM_ROOT.'core/sys.php');
require(ASM_ROOT.'core/provider.php');

require(ASM_ROOT.'core/endpoint.php');
require(ASM_ROOT.'core/http.php');
require(ASM_ROOT.'core/cli.php');

require(ASM_ROOT.'core/config.php');
require(ASM_ROOT.'core/app.php');
require(ASM_ROOT.'core/request.php');


require(ASM_ROOT.'core/exception.php');

require(ASM_ROOT.'core/template.php');


// Load the default en-US locale
// @todo update according to FAIQ - probably require promptd
# require(ASM_ROOT.'core/enUS.php');

