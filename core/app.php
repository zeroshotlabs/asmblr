<?php declare(strict_types=1);
/**
 * @file app.php Application runtime.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;
use asm\types\url;


/**
 * The primary application runtime.  This centrally provides access to the
 * runtime environment, and manages the application, including caching, configuration,
 * startup, routing, etc., and is the default context for endpoint execution.
 */
class app
{
    /**
     * @var string $CacheDir
     * Absolute path to the asmblr local cache.
     *
     * @note This must be set explicitly in the extending class.
     *
     * @todo For future GAE integration:
     *      @c gs://asmblr-mc-tmp/
     *      @c array('gs'=>array('Content-Type'=>'text/plain'))
     */
    public $CacheDir = '';
    /**
     * @var boolean $CacheManifest
     * TRUE to cache the manifest to the local disk.
     * 
     * @note This is configured in the application array in @c index.php
     */
    public $CacheManifest = FALSE;

    /**
     * @var boolean $CacheApp
     * TRUE to cache the app into a local app-file.
     *
     * @note This is configured in the application array in @c index.php
     */
    public $CacheApp = FALSE;

    public readonly bool $IsProduction;

    protected $base_url;

    protected $exec_endpoint;

    public function __construct( protected \asm\config $config,protected \asm\request $request )
    {
        $this->IsProduction = $this->config->IsProduction;
    }

    /**
     * Routing is performed using explicit matching:
     *  - loop over the requested URL path, from root down
     *  - each increasing URL is matched as a directory with trailing slash (IsDir = true)
     *  - finally a leaf (IsDir = false) is matched
     * 
     * This means that the trailing slash is important and URLs are matched explicitly.
     * For example:
     *  - given a request of /admin, an endpoint with /admin but not /admin/ will match
     *  - given a request of /admin/users, endpoints with /admin/ and /admin/users will match.
     *  - given a request of /admin/users/username, endpoints /admin/, /admin/users/ will match.
     *    /admin and /admin/users will not.
     * 
     * The super-root (//) is always executed first.
     * 
     * @return array $exec_q The Endpoints to execute, in order.
     * @todo Abstract this out as it's own closure/class for custom routers
     */
    public function route(): array
    {
        $exec_q = [];
        $exec_q[] = $this->config->endpoints_url_map['//'];

        foreach( $this->request->route_path->ordered() as $path )
        {
            if( isset($this->config->endpoints_url_map[$path]) )
                $exec_q[] = $this->config->endpoints_url_map[$path];
        }

        return $exec_q;
    }

    public function execute_endpoint( array $endpoint )
    {
        var_dump($endpoint);

    }
}

