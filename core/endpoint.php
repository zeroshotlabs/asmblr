<?php declare(strict_types=1);
/**
 * @file endpoint.php Associates a URL or named request to an application logic block.
 * @author @zaunere Zero Shot Labs
 * @version 5.0
 * @copyright Copyright (c) 2023 Zero Shot Laboratories, Inc. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;

/**
 * This base class implements default routing for asmblr using __invoke(), as described below.
 * 
 * Endpoints can serve two purposes:
 *  - Routing Endpoint: Used to route requests URLs to other endpoints, or execute logic for a
 *    hierarchy of URLs.
 *      - The 'root' has URL '//' and performs the default routing for web requests.
 *        All leaf-routing could be done here, with no other configured endpoints.
 *      - If an endpoint's path has a trailing slash and matches the requests' leading path parts,
 *        an endpoint object is instantiated and executed (__invoke())),
 *        for example, an endpoint configured as /admin/ will match requests under it, like /admin/user-list
 *        it will NOT match /admin unless suffixed with a plus, like /admin+ @todo double check
 *      - Specified in configuration as:  endpoint_class
 *      - If a routing endpoint returns false, no other endpoints are executed and the
 *        response finishes.  @todo
 *      - Routing endpoints should implement __construct() and __invoke().
 * 
 *  - Leaf Endpoint: Used to execute application logic blocks for specific, or leaf, URLs.
 *      - If an endpoint's path doesn't have a trailing slash and matches the request's full path,
 *        an endpoint obj is instantiated and the method specified in the config is called.
 *      - Specified in configuration as:  endpoint_class::url_method
 *      - Leaf endpoints should implementment __construct() and various URL-specific methods.
 * 
 * Both endpoint types are reused by default. In both cases, endpoint objects are reused, meaning they are instantiated once.
 * 
 * @note CLI requests execute endpoints by name and the above doesn't apply.
 * 
 * @todo interfaces are superfluous here at the moment.
 * @todo force new endpoint objects to be created, rather than reusing; might be better in app.
 */

interface routing_endpoint { }

interface leaf_endpoint extends routing_endpoint { }
 
 
abstract class endpoint
 // implements leaf_endpoint
{
    protected $app;
    protected $exec_ep;
    protected $request;

    public function __construct( \asm\app $app )
    {
        $this->app = $app;
//        $this->exec_ep = $endpoint;
    }

    /**
     * Call this method, parent::__invoke(), at the beginning
     * of the child method.  Or not.
     */
    public function __invoke( \asm\request $request,array $endpoint )
    {
        $this->request = $request;
        $this->exec_ep = $endpoint;
    }

    /**
     * Call this method, parent::_leaf(), at the beginning of an
     * child leaf method.  Or not.
     */
    protected function _leaf( \asm\request $request,array $endpoint )
    {
        $this->request = $request;
        $this->exec_ep = $endpoint;
    }
 }

 
// *  - exclude page from session and zlib compression in sitewide.php

class root extends endpoint
{
//    public

}


class http extends root
{

}


class cli extends root
{
//    public

}

