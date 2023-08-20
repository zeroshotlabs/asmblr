<?php
/**
 * @file Endpoint.php Associates a URL or named request to an application logic block.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


class Endpoint
// implements Endpointi
{
    protected $_func = NULL;

    private function __construct( $func )
    { }

    public function __invoke( $request,$target )
    {

    }
}


// $dd->login();
// $dd->login();


// /**
//  * An Endpoint represents a unit of application logic for a Path or hierarchy of Paths, or by name.
//  *
//  * An Endpoint can be matched in one of the following ways:
//  * 
//  *  contains one trigger absolute Path, a Name, an optional function, and none, one or more
//  * Directives.  The Path and Name must be unique within a PageSet.
//  *
//  * Pages are executed and managed by a PageSet, forming the "control layer".  PageSets and
//  * Pages are managed through the config.
//  */
// abstract class Page extends DAO
// {
//     /**
//      * @var array $Skel
//      * The base structure of a Page.
//      */
//     protected static $Skel = array('Name'=>'','Path'=>'','Status'=>'','PathStruct'=>array(),'Function'=>array(),'Directives'=>array());


//     /**
//      * Create a new Page Struct.
//      *
//      * @param string $Name The name of the Page.
//      * @param string $Path The trigger absolute path of the Page which is lowercased.
//      * @param string $Status The page's status, generally @c Active.
//      * @param string|array $Function A function callback.
//      * @retval array The created Page Struct.
//      *
//      * @note The Path should not be encoded.
//      */
//     public function __construct( protected $Name,protected $Path, protected $Status,protected $Function = NULL )
//     {
//         $Page = static::$Skel;

//         $Page['Name'] = $Name;

//         $Page['Path'] = strtolower($Path);
//         $Page['PathStruct'] = Path::str($Page['Path']);

//         $Page['Status'] = $Status;

//         $Page['Function'] = $Function;

//         return $Page;
//     }
// }
