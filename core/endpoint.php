<?php declare(strict_types=1);
/**
 * @file Endpoint.php Associates a URL or named request to an application logic block.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;

use asm\types\endpointi as endpointi;

/**
 * 
 * This base class implements default routing for asmblr using __invoke(), as described below.
 * 
 * Endpoints can serve two purposes:
 *  - they can be used to route URLs to other endpoints, or execute logic for a
 *    hierarchy of URLs
 *      - the 'root' has URL '//' and performs the default routing for web requests.
 *      - if the configured path contains a trailing slash, an endpoint object is 
 *        instantiated and executed (__invoke()) 
 *      - configured as:  endpoint_class
 *      - if a routing endpoint returns false, no other endpoints are executed and the
 *        response finishes.
 * 
 *  - they can be used to execute application logic blocks for specific URLs
 *      - if the configured path doesn't contain a trailing slash, an endpoint obj 
 *        is instantiated and the specified method is called. 
 *      - configured as:  endpoint_class::url_method
 * 
 * As is general practice, non-routers should implement the constructor
 * and a method specific to the request, which is defined in the config.  Routers invoke __invoke().
 * 
 * None of this applies to CLI requests as a single endpoint class is executed by name.
 */
class endpoint implements endpointi
{
    protected $_exec = NULL;

    private function __construct( array $endpoint )
    {
        $this->_exec = $endpoint;
    }

    public function __invoke( $request,$target )
    {

    }
}

class root extends endpoint
{
//    public

}