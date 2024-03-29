<?php declare(strict_types=1);
/**
 * @file app.php Application runtime.
 * @author @zaunere Zero Shot Labs
 * @version 5.0
 * @copyright Copyright (c) 2023 Zero Shot Laboratories, Inc. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License v3.0 or later.
 * @copyright See COPYRIGHT.txt.
 */
namespace asm;
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
    public bool $is_production;

    /**
     * Automatically true if the request is from the command line.
     */
    public bool $is_cli;

    /**
     * True if the current request is a root request (not super root).
     */
    public bool $is_root;

    /**
     * True if the current request is a FES request.
     */
    public bool $is_fes;

    /**
     * The super root endpoint or null.
     * 
     * These are executed for CLI requests as well.
     */
    public array $super_root;

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
        $this->is_production = $this->config->is_production;
        $this->is_cli = $this->request->is_cli;

        $this->super_root = $this->config->endpoints_url_map['//'] ?? [];
    }


    /**
     * Determine what to execute.
     * 
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
     * The super-root (//) is always executed first by default for both CLI and web.
     * 
     * Similarly, double trailing slashes of an endpoint will make it execute
     * for directory and non-directory (leaf) requests, i.e.
     *  - request: /admin/anything or /admin or /admin/
     *    matches: /admin// will match all
     * 
     * These are coined "greedy" roots/routes.
     * 
     * @return array &$route_table Reference to the endpoints execution queue, in order.
     * @todo Abstract this out as it's own closure/class for custom routers; for now extend the class.
     * @note Be minful when using greedy roots on top-level endpoints, including FES requests.
     * @note This should be called once.
     */
    public function &route(): array
    {
        if( !empty($this->super_root) )
            $this->route_table[] = $this->super_root;

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
    public function exec_cli( string|array $endpoint ): mixed
    {
        if( is_string($endpoint) )
            $endpoint = $this->config->endpoints[$endpoint]
                ?? throw new e500("CLI endpoint '$endpoint' not found.");

        if( !$this->is_cli )
            _stde("Warning: '{$endpoint['name']}' exec_cli requested but not running on CLI.");

        $endpoint = $this->config->endpoints[$endpoint['name']]
            ?? throw new e500("CLI endpoint '{$endpoint['name']}' not found.");
        
        !empty($endpoint['url'])
            ?? _stde("Warning: '{$endpoint['name']}' exec_cli requested endpoint has URL '{$endpoint['url']}'");

        return $this->_exec_endpoint($endpoint);
    }


    /**
     * Execute a web endpoint by name.
     * 
     * This only warns if a incorrectly mixed web/CLI call is made.
     * 
     * @param string $name The name of the endpoint to execute, that has a URL.
     */
    public function exec_web( string|array $endpoint ): mixed
    {
        if( is_string($endpoint) )
            $endpoint = $this->config->endpoints[$endpoint]
                ?? throw new e500("Web endpoint '$endpoint' not found.");

        $this->is_cli
            ?? _stde("Warning: '{$endpoint['name']}' exec_web requested but not running on the web.");

        empty($endpoint['url'])
            ?? _stde("Warning: '{$endpoint['name']}' exec_web requested endpoint has no URL");

        return $this->_exec_endpoint($endpoint);
    }


    /**
     * Execute a single endpoint by name or definition array.
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
    public function _exec_endpoint( array|string $endpoint ): mixed
    {
        if( is_string($endpoint) )
        {
            if( isset($this->config->endpoints[$endpoint]) )
                $endpoint = $this->config->endpoints[$endpoint];
            else
                throw new e500("Endpoint '$endpoint' not found.");
        }

        // @note Execute a routing or CLI endpoint which is instantiated and __invoked()'d.
        //       The executing object persists in $this->route_table[$exec[0]]['obj'] and will be
        //       reused if nessecary.
        // @todo check namespaces.  this doesn't actually care about interface implementation and
        //       probably breaks across namespaces
        if( count($endpoint['exec']) === 1 )
        {
            if( !isset($this->route_table[$endpoint['exec'][0]]['obj']) )
            {
                $e = "\\{$endpoint['exec'][0]}";                
                $exec_obj = $this->route_table[$endpoint['exec'][0]]['obj'] = new $e($this);
            }
            else
                $exec_obj = $this->route_table[$endpoint['exec'][0]]['obj'];

            return $exec_obj($this->request,$endpoint);
        }
        // @note Execute a leaf endpoint which is a method call.
        else if( count($endpoint['exec']) === 2 )
        {
            if( !isset($this->route_table[$endpoint['exec'][0]]['obj']) )
            {
                $e = "\\{$endpoint['exec'][0]}";
                $exec_obj = $this->route_table[$endpoint['exec'][0]]['obj'] = new $e($this);
            }
            else
                $exec_obj = $this->route_table[$endpoint['exec'][0]]['obj'];

            return $exec_obj->{$endpoint['exec'][1]}($this->request,$endpoint);
        }
        else
            throw new e500("Malformed endpoint exec for '{$endpoint['name']}");
    }

}

