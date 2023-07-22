<?php
/**
 * @file DAO.php Data access object (supersedes struct).
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * General purpose data objects.
 * 
 * @todo supersede array based struct across the board (asmblr v6).
 * @todo implement full features like debug/etc 
 * @todo revisit dynamic properties (again)
 */
#[\AllowDynamicProperties]
abstract class DAO implements Debuggable,\ArrayAccess
{
    use Debugged;
    use DAOt;

}


