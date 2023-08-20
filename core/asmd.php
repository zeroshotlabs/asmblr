<?php
/**
 * @file asmd.php Application runtime controller.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * The primary application runtime object.  This centrally provides access to the
 * runtime environment, and manages the application, including caching, configuration,
 * startup, routing, etc., and is the default context for endpoint execution.
 */
class asmd
{
    /**
     * @var Page $OrderedMatch
     * The Page Struct that that matched a hierarchal path (those with a trailing @c / in the URL) ie., an ordered match.
     *
     * @note Read-only.  May be empty.
     */
    public $OrderedMatch;

    /**
     * @var Page $ExactMatch
     * The Page Struct that was an exact match (those withot a trailing @c / in the URL) that matched the request.
     *
     * @note Read-only.  May be empty.
     */
    public $ExactMatch;


    /**
     * @var string $ClosestMatchName
     * The Page name of the page that most closely matches the request.
     *
     * If there is an exact match, this will be the same as App::$ExactMatch, otherwise it will
     * be App::$OrderedMatch.
     *
     * @note Read-only.  May be empty only if there's no matches (404).
     */
    public $ClosestMatchName;

    /**
     * @var asm::Request $Request
     * The Request Struct which contains normalized details about the current HTTP or CLI request.
     *
     * @note Read-only.
     */
    public readonly \asm\Request $request;

    /**
     * @var array $Manifest
     * The application's manifest built from it's Google Spreadsheet.
     *
     * The manifest contains application configuration, including pages/routes, templates and misc.
     * configuration settings.
     *
     * @note Read-only.
     */
    public $Manifest;

    /**
     * @var array $Pages
     * A listing page Pages by name.
     *
     * This contains pages for each PageSet tab defined by the manifest.
     *
     * @note Read-only.
     */
    public $Pages;

    /**
     * @var array $PageMaps
     * A mapping of Page paths (URL) to their Page Name.
     *
     * This contains all page mappings; one for each PageSet tab defined by the manifest.
     *
     * @note Read-only.
     */
    public $PageMaps;

    /**
     * @var array $Config
     * Key/value config directives defined by the Config tab in the manifest.
     *
     * A key with two or more values (each it's own column) will have an array value.
     *
     * @note Read-only.
     */
    public $Config;

    /**
     * @var array $Templates
     * A listing of Templates as found in the @c templates directory and defined by the manifest.
     *
     * @note Read-only.
     */
    public $Templates;


    protected function sweeten_request_path( string $path )
    {
        return array_filter(explode('/', strtolower($path)));
    }

    public function __construct()
    {
        $this->request = new \asm\Request;
        echo 'sds';


        $c = new \asm\config('1tQSC60yxj8BUFSm0GBmIEziQBMpXiDnYlfbpRX8roos',['asma','azoai','creds2']);

        $asmd = new \asm\asmd;
        $GLOBALS['asmd'] = $asmd;
        
        
        $login = function( $request,$target )
        {
            echo 'here we are in'.__CLASS__;
            var_dump($this->hello);
        };
        
        
        class promptd_endpoint extends \asm\Endpoint
        {
            private $hello = 'world';
            public $kb;
        
            public function __construct( \closure $kb )
            {
                $this->kb = $kb;
            }   
        }
        
        $dd = new promptd_endpoint($login);
        $hi = $GLOBALS['asmd']->bindTo($dd,$dd);
        
        $hi([],[]);
        
        // var_dump($c);
        
        

    }
}


// private function isHierarchicalMatch($request, $url) {
//     $urlLength = strlen($url);
    
//     if (strncasecmp($request, $url, $urlLength) === 0) {
//         // Exact or hierarchical match
//         if ($request[$urlLength - 1] === '/' || $request[$urlLength] === '/' || $url[$urlLength - 1] === '/') {
//             // Trailing slash match
//             return true;
//         }
//     }
    
//     return false;
// }









// class Router {
//     private $routes = [];

//     public function addRoute($url, $handler) {
//         $this->routes[$this->normalizePath($url)] = $handler;
//     }

//     public function matchRequest($request) {
//         $normalizedRequest = $this->normalizePath($request);

//         // Exact match
//         if (isset($this->routes[$normalizedRequest])) {
//             return [$this->routes[$normalizedRequest]];
//         }

//         // Hierarchical matches
//         $matchedPages = [];
//         foreach ($this->routes as $url => $handler) {
//             if ($this->isHierarchicalMatch($normalizedRequest, $url)) {
//                 $matchedPages[] = $handler;
//             }
//         }

//         return $matchedPages;
//     }

//     private function isHierarchicalMatch($request, $url) {
//         return strpos($request, $url) === 0;
//     }
// }




// class Router {
//     private $routes = [];

//     public function addRoute($url, $handler) {
//         $this->routes[] = ['url' => $url, 'handler' => $handler];
//     }

//     public function matchRequest($request) {
//         $normalizedRequest = $this->normalizePath($request);
//         $matchedPages = [];

//         foreach ($this->routes as $route) {
//             $url = $route['url'];
//             if ($this->isHierarchicalMatch($normalizedRequest, $url)) {
//                 $matchedPages[] = $route['handler'];
//             }
//         }

//         return $matchedPages;
//     }


//     private function isHierarchicalMatch($request, $url) {
//         return strpos($request, $url) === 0;
//     }
// }

