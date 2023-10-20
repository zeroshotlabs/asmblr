<?php declare(strict_types=1);
/**
 * @file app.php Application runtime.
 * @author @zaunere Zero Shot Labs
 * @version 5.0
 * @copyright Copyright (c) 2023 Zero Shot Laboratories, Inc. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;
use asm\types\url;
use asm\sys;
use asm\http;
use asm\cli;
use asm\_e\e500;

use function asm\sys\_stde;


/**
 * The primary application runtime.
 * 
 * This centrally provides access to the runtime environment, comprised primarily
 * of routing, execution/endpoint, and template/out.
 * 
 * This also holds configuration parameters, and provides access to the request,
 * and is the default context for endpoint execution.
 * 
 * @todo add linker for endpoints/FES
 */ 
abstract class app
{
//    use linker;
    /**
     * True if the app is configured 'live' or otherwise determined as such.
     */
    public bool $IsProduction;

    /**
     * Automatically true if the request is from the command line.
     */
    public bool $IsCLI;

    /**
     * True if the current request is a root request (not super root).
     */
    public bool $IsRoot;

    /**
     * True if the current request is a FES request.
     */
    public bool $IsFES;

    /**
     * The super root endpoint or null.
     * 
     * These are executed for CLI requests as well.
     */
    public array|null $super_root;

    /**
     * The route table controls app execution according to matched
     * web endpoints.
     * 
     * @note Does not include FES.
     * @see self::route()
     */
    public array $route_table = [];    


    /**
     * @todo add config overrides Key/value array of custom application config variables from
     * index.php, that override config.
     */
    public function __construct( public \asm\config $config,public \asm\request $request )
    {
        $this->IsProduction = $this->config->IsProduction;
        $this->IsCLI = $this->request->IsCLI;

        $this->super_root = $this->config->endpoints_url_map['//'] ?? NULL;
    }


    /**
     * Builds the route_table (execution queue) using explicit matching:
     *  - loop over the requested URL path, from root down
     *  - each increasing URL is matched as a directory with trailing slash (IsDir = true)
     *  - finally a leaf (IsDir = false) is matched
     * 
     * This means that the trailing slash is important and URLs are matched explicitly.
     * For example:
     *  - request: /admin
     *    matches: /admin but not /admin/
     *  - request: /admin/users
     *    matches: /admin/ and /admin/users but not /admin nor /admin/users/
     *  - request: /admin/users/username
     *    matches: /admin/ and /admin/users/ but not /admin nor /admin/users
     *  
     * The super-root (//) is always executed first.
     * 
     * Similarly, double trailing slashes of an endpoint will make it execute
     * for directory and non-directory (leaf) requests, i.e.
     *  - request: /admin/anything or /admin or /admin/
     *    matches: /admin// will match all
     * 
     * These are coined "greedy" roots.
     * 
     * @return array &$route_table Reference to the endpoints execution queue, in order.
     * @todo Abstract this out as it's own closure/class for custom routers.
     * @note This is for ALL web requests and will determine if it's a FES request.
     * @note Be minful when using greedy roots on high endpoints.
     * @note This should be called once.
     */
    public function &route(): array
    {
        foreach( $this->request->route_path->ordered() as $path )
        {
            if( isset($this->config->endpoints_url_map[$path.'//']) )
                $this->route_table[] = $this->config->endpoints_url_map[$path.'//'];

            if( isset($this->config->endpoints_url_map[$path])  )
                $this->route_table[] = $this->config->endpoints_url_map[$path];
            else if( isset($this->config->endpoints_url_map[$path.'/']) )
                $this->route_table[] = $this->config->endpoints_url_map[$path.'/'];
        }

        return $this->route_table;
    }

    /**
     * Execute a CLI endpoint by name.
     * 
     * This only warns if a incorrectly mixed web/CLI call is made.
     * 
     * @param string $Name The name of the endpoint to execute which
     *               shouldn't have a configured URL.
     */
    public function exec_cli( string $name ):mixed
    {
        !$this->IsCLI
            ?? _stde("Warning: '$name' exec_cli requested but not running on CLI.");

        $endpoint = $this->config->endpoints[$name]
            ?? throw new e500("CLI endpoint '$name' not found.");
        
        !empty($endpoint['url'])
            ?? _stde("Warning: '$name' exec_cli requested endpoint has URL '{$endpoint['url']}'");

        return $this->exec_endpoint($endpoint);
    }


    /**
     * Execute a single endpoint.
     * 
     * This is used to execute a single endpoint.  If there is a period in the 'exec'
     * element, it's split.
     *  - if there are two elements, the first is taken as the class to instantiate (__construct() is thus called)
     *    and the second as the method to call. 
     *  - if there is only a single element, it is taken as the class to instantiate (__construct() is called)
     *    and execute (__invoke() is called).
     * 
     * @param array|string $endpoint An endpoint array or name of an endpoint in the current config.
     * 
     * @note app classes are assumed in the global namespace of the app (see $e below).
     */
    public function exec_endpoint( array|string $endpoint ): mixed
    {
        if( is_string($endpoint) )
        {
            if( isset($this->config->endpoints[$endpoint]) )
                $endpoint = $this->config->endpoints[$endpoint];
            else
                throw new e500("Endpoint '$endpoint' not found.");
        }

        $exec = explode('.',$endpoint['exec']??'');

        // @note Execute a routing or CLI endpoint which is instantiated and __invoked()'d.
        //       The executing object persists in $this->route_table[$exec[0]]['obj'] and will be
        //       reused if nessecary.
        // @todo check namespaces.  this doesn't actually care about interface implementation.
        if( count($exec) == 1 )
        {
            if( !isset($this->route_table[$exec[0]]['obj']) )
            {
                $e = "\\$exec[0]";                
                $exec_obj = $this->route_table[$exec[0]]['obj'] = new $e($this);
            }
            else
                $exec_obj = $this->route_table[$exec[0]]['obj'];

            return $exec_obj($this->request,$endpoint);
        }
        // @note Execute a leaf endpoint.
        else if( count($exec) == 2 )
        {
            if( !isset($this->route_table[$exec[0]]['obj']) )
            {
                $e = "\\$exec[0]";
                $exec_obj = $this->route_table[$exec[0]]['obj'] = new $e($this);
            }
            else
                $exec_obj = $this->route_table[$exec[0]]['obj'];

            return $exec_obj->{$exec[1]}($this->request,$endpoint);
        }
        else
            throw new e500("Malformed endpoint exec '{$endpoint['exec']}");
    }
}


