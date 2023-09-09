<?php
/**
 * @file load.inc CORS drop-in transparent proxy.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm\promptd;

define('ASM_EXT_CORSP_ROOT',__DIR__.DIRECTORY_SEPARATOR);

require(ASM_EXT_CORSP_ROOT.'corsp.inc');

// @todo fold this in - was from HTTP
    // /**
    //  * Send default headers to support CORS, including handling an OPTIONS request.
    //  *
    //  * This allows from any origin, GET/POST/OPTIONS methods and most headers.
    //  *
    //  * @todo This currently supports only generic default behavior.  Needs parameters to fine tune/restrict.
    //  *
    //  * @see http://enable-cors.org/server_nginx.html for nginx handling.
    //  * @see https://developer.mozilla.org/en-US/docs/Web/HTTP/Access_control_CORS
    //  */
    // public static function CORS()
    // {
    //     // support OPTIONS pre-flight with any origin and exit
    //     if( $_SERVER['REQUEST_METHOD'] === 'OPTIONS' )
    //     {
    //         if( !empty($_SERVER['HTTP_ORIGIN']) )
    //             header("Access-Control-Allow-Origin: {$_SERVER['HTTP_ORIGIN']}");

    //         header('Access-Control-Allow-Credentials: true');
    //         header('Access-Control-Max-Age: 25');    // cache for 25 seconds
    //         header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
    //         header('Access-Control-Allow-Headers: Authorization,Content-Type,Accept,Origin,User-Agent,DNT,Cache-Control,X-Mx-ReqToken,Keep-Alive,X-Requested-With,If-Modified-Since');

    //         header('Content-Length: 0');
    //         HTTP::_204();
    //     }
    //     // allow from any origin
    //     else if (isset($_SERVER['HTTP_ORIGIN']))
    //     {
    //         header('Access-Control-Allow-Credentials: true');
    //         header('Access-Control-Max-Age: 25');    // cache for 25 seconds
    //         header("Access-Control-Allow-Origin: {$_SERVER['HTTP_ORIGIN']}");
    //     }
    // }
