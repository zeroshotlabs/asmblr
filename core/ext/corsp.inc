<?php
/**
 * @file corsp.inc CORS transparent proxy.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * CORS drop-in "transparent" proxy.
 *
 * This function proxies local requests to a remote API so that CORS isn't an issue.
 *
 * Install:
 *  - CORSP_DestURL config variable must be set in the manifest's config, which can be overridden.
 *  - a page - commonly corsp - mapped to this function with a path such as /ajx/ as shown below.
 *    The path must be only a single segment.
 *  - exclude page from session and zlib compression in sitewide.php
 *
 * This function should be defined in the manifest as a Page's function, such as:
 *  @code
 *  corsp	/ajx/	Active	\asm\corsp
 *  @endcode
 *
 * Note that the namespace is required.
 *
 * This will pass nearly all headers - see code for details and exceptions.
 *
 * @todo We may choose to extend restr at some point, rather than use generic.
 * @todo Fully test, including GET method, and possibly add config options.
 */
function corsp( $app )
{
    // must have a method present
    if( empty($_SERVER['REQUEST_METHOD']) )
        \asm\HTTP::_400();

    // determine our Ajax method URL since it's dynamic. it's everything past our first segment
    $ProxyPath = $app->Request['MatchPath'];
    \asm\Path::Del(0,$ProxyPath);

    // list of request headers to pass-thru to the destination
    $PassHeaders = array('origin','user-agent','content-type','accept',
                         'accept-encoding','accept-language','cookie');

    $Headers = array();
    foreach( getallheaders() as $K => $V )
    {
        if( in_array(strtolower($K),$PassHeaders,TRUE) )
            $Headers[$K] = $V;
    }

    // always added
    $Headers['Connection'] = 'close';

    $rest = new \asm\restr($app->Config['CORSP_DestURL'],array('CORSP'=>\asm\Path::ToURLString($ProxyPath)));

    if( $_SERVER['REQUEST_METHOD'] === 'GET' )
    {
        $R = $rest->GET('CORSP',$_GET,TRUE,$Headers);
    }
    else if( $_SERVER['REQUEST_METHOD'] === 'POST' )
    {
        // sending raw post as a string means our proxied content-type must be correct
        $R = $rest->POST('CORSP',file_get_contents('php://input'),!empty($_GET)?$_GET:array(),TRUE,$Headers);
    }
    else
    {
        llog("Unknown method for corsp: {$_SERVER['REQUEST_METHOD']}");
        \asm\HTTP::_400();
    }

    // if we don't get anything back, this is a no-content
    if( empty($R) )
        \asm\HTTP::_204();

    // we passback only access-control, cookie and content-type - others?  encoding?
    foreach( $rest->Headers as $K => $V )
    {
        if( stripos($K,'access-control') !== FALSE )
            header("{$K}: {$V}",TRUE);
        else if( stripos($K,'cookie') !== FALSE )
            header("{$K}: {$V}",TRUE);
        else if( stripos($K,'content-type') !== FALSE )
            header("{$K}: {$V}",TRUE);
    }

    echo $R;
    exit;
}

