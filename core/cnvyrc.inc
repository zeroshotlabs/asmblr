<?php
/**
 * @file cnvyr.inc cnvyr asset/media delivery and manipulation.
 * @author Stackware, LLC
 * @version 4.0
 * @copyright Copyright (c) 2012-2014 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * cnvyr client caches and delivery of assets using the cnvyr.io API.
 *
 * It is recommended that header_remove() be called for all cnvyr pages.
 *
 * All caching is done to a local filesystem path.
 *
 * @todo Possibly add helper methods for HTTP caching, storing cache files, gzip, etc.
 * @todo Determine passthru/no-op behavior - i.e. just cache so it can be sent via x-sendfile.
 * @todo And passthru/proxy behavior to avoid CORS issues. perhaps this could even be a paid serrvice.
 * @todo If we have a gzip op, we shoul be caching the gzip'd version and then directly serving that
 *       with the proper headers.
 */
abstract class cnvyrc extends restr
{
    // configuration required by extending class
    // TODO: origin dir may be deprecated in favor of always using file content as strings
    public $OriginDir = '';
    public $CacheDir = '';
    public $CacheLocal = FALSE;
    public $BaseURL = '';

    protected $Methods = array('v1'=>'/v1');

    // Base URL path for nginx's X-Accel-Redirect - needs leading and trailing slashes
    // Must match nginx config (see nginx-asmblr.conf)
    protected static $XAccelRedirectBase = '/xcnvyr/';


    public function __construct( $BaseURL = '',$Methods = array() )
    {
        global $asmapp;

        parent::__construct($BaseURL,$Methods);

        // setup cache dir if relative to app's CacheDir
        if( strpos($this->CacheDir,'/') === FALSE )
            $this->CacheDir = "{$asmapp->CacheDir}/{$this->CacheDir}/";
    }

    /**
     * Serve an asset or a bundle of assets.
     *
     * The handler is prepended to the cached resource's filename.
     *
     * A bundle is one or more local files and/or string $Content.  If $Content is a string,
     * it's taken as the content of a single file, $Filename.  If $Content is empty, $Filename
     * is taken as a local file.
     *
     * If $Content is an array, it will be taken as a bundle of local files and/or file content
     * where each non-numeric key is the filename of it's content as the value.  Numeric keys are
     * treated as filenames.  The bundled resource will then be cached as $Filename.
     *
     * This does not send a Content-Type header.
     *
     * @param string $Handler Handler for resource.  Must be unique for different variations of the same resource.
     * @param string $Filename Resource's filename or placeholder when using bundles.
     * @param array $Ops Associative array of ops.
     * @param string $Content File content to send as $Filename.
     * @param array $Content Array of content and filenames.
     *
     * @note This could be executed in Load.inc to avoid the asmblr startup (with some config tweaks).
     * @note This exits.
     * @note A group of ops should have a unique OpToken otherwise cache collisions will occur.
     * @note An empty Ops parameter will result only in caching/combining the asset(s).
     */
    public function srv( $Handler,$Filename,$Ops = array(),$Content = '' )
    {
        // pull the raw resource and cache if allowed
        // NOTE: For a premium version, we'd allow a remote origin (URL)

        // treat as a bundle - if one file isn't found, a 404 is kicked out
        if( is_array($Content) === TRUE )
        {
            $Bundle = array();
            $i = 0;
            foreach( $Content as $K => $V )
            {
                // local filename
                if( is_int($K) === TRUE )
                {
                    if( is_readable($this->OriginDir.$V) === FALSE )
                    {
                        HTTP::_404();
                        exit;
                    }

                    $Bundle['files'.$i] = $this->OriginDir.$V;
                }
                // file content
                else
                {
                    $Bundle['@files'.$i] = $V;
                }

                ++$i;
            }

            $Response = $this->POSTFiles('v1',$Ops,array(),$Bundle);
        }
        // single file, either from filesystem or buffer
        else
        {
            if( empty($Content) )
            {
                // check if local file exists - the requested URL is always lowercased
                // thus origin assets must also be lowercased
                if( is_readable($this->OriginDir.$Filename) === FALSE )
                {
                    HTTP::_404();
                    exit;
                }

                $Response = $this->POSTFiles('v1',$Ops,array(),array('files0'=>$this->OriginDir.$Filename));
            }
            else
            {
                $Response = $this->POSTFiles('v1',$Ops,array(),array('@files0'=>$Content));
            }
        }

        if( !empty($this->CURLError) )
        {
            echo $Response;
            HTTP::_500();
        }

        // TODO: using gzencode() is hardwired; all cached resources will be compressed
        // this probably should be configurable - in cache too
        if( $this->CacheLocal === TRUE )
            file_put_contents($this->CacheDir.$Handler.'_'.$Filename,gzencode($Response));

        echo $Response;
        exit;
    }

    /**
     * Cache and serve generic local files and buffers using optimized web server techniques.
     *
     * This includes x-senfile/x-acell/etc serving.
     *
     * This does not support any Ops and only a single file.
     *
     * @see cnvyrc::srv()
     *
     * @todo This may all need/should be refactored.
     */
    public function cache( $Handler,$Filename,$Content = '' )
    {
        if( empty($Content) )
        {
            // check if local file exists - the requested URL is always lowercased
            // thus origin assets must also be lowercased
            if( is_readable($this->OriginDir.$Filename) === FALSE )
            {
                HTTP::_404();
                exit;
            }

            $Response = file_get_contents($this->OriginDir.$Filename);
        }
        else
        {
            $Response = $Content;
        }

        if( $this->CacheLocal === TRUE )
            file_put_contents($this->CacheDir.$Handler.'_'.$Filename,gzencode($Response));

        echo $Response;
        exit;
    }


    /**
     * Attempt a cache hit and serve the cached resource, if available.
     *
     * @param string $Filename The filename of the resource to be checked and served.
     * @param string $CacheDir The full path to the cache directory for cnvyr with trailing slash.
     * @retval boolean TRUE if the cache hit was successfully served.
     *
     * @note With some special handling, this can be called from index.php, avoiding
     *       the rest of asmblr execution.
     * @note $Filename and $CacheDir are trusted - do checks elsewhere.
     * @todo Possible override to output directly without using xaccel/sendfile
     */
    public static function CacheHit( $Filename,$CacheDir )
    {
        if( is_readable($CacheDir.$Filename) === FALSE )
            return FALSE;

        // nginx
        if( PHP_SAPI === 'fpm-fcgi' )
        {
            // nginx generally also sets correct etag/last-modified headers based on the cache file.
            // note that if the extension of the file isn't correct, nginx will fail to send the correct content type
            // required so that nginx sets the correct header.
            header_remove('Content-Type');
            header('X-Accel-Redirect: '.static::$XAccelRedirectBase.$Filename);
        }
        // apache2 - assumes mod_xsendfile has been configured
        else if( PHP_SAPI === 'apache2handler' )
        {
            // should support three and four letter extensions
            HTTP::ContentType(ltrim(substr($Filename,-4),'.'));

            // TODO: need content length?
            header("X-SendFile: {$CacheDir}{$Filename}");
        }
        // everything else - IIS (cgi-fcgi)/GAE/etc
        else
        {
            // should support three and four letter extensions
            HTTP::ContentType(ltrim(substr($Filename,-4),'.'));

            // TODO: also sending content length a good idea?
            readfile($CacheDir.$Filename);
        }

        return TRUE;
    }


    /**
     * Parse a path into cnvyr components.
     *
     * This requires a Path Struct, typically the request's MatchPath, and expects a path in the form:
     *   /handler-url/{optional-cache-buster}/filename.ext | bundle-token
     *
     * A cache-buster is generally otherwise ignored in serving the asset.
     *
     * The resource filename/bundle-token is rawurldecode()'d and stripped of '..' and '/' characters.
     *
     * @param array $P Path Struct to parse.
     * @retval array Numeric array with handler, filename/bundle and cache buster elements.
     * @retval NULL The path could not be parsed.
     */
    public static function PathParse( $P )
    {
        // /handler-url/cache-buster pattern
        if( !empty($P['Segments'][2]) )
        {
            $CacheBuster = $P['Segments'][1];
            $Handler = str_replace(array('..','/'),'',rawurldecode($P['Segments'][0]));
            $Filename = str_replace(array('..','/'),'',rawurldecode($P['Segments'][2]));
        }
        // /handler/filename pattern
        else if( !empty($P['Segments'][1]) )
        {
            $Filename = str_replace(array('..','/'),'',rawurldecode($P['Segments'][1]));
            $Handler = str_replace(array('..','/'),'',rawurldecode($P['Segments'][0]));
            $CacheBuster = '';
        }
        // nothing (probably a 404)
        else
        {
            return array();
        }

        return array($Handler,$Filename,$CacheBuster);
    }
}



/**
 * Create URLs for cnvyr handler pages.
 *
 * @note Largely similar to LinkPage.  Modifications to __invoke().
 */
class Linkcnvyr extends LinkPage
{
    /**
     * Linkcnvyr constructor.
     *
     * @param PageSet $PageSet PageSet containing Pages to create URLs for.
     * @param array $BaseURL URL Struct to use as BaseURL.
     * @param string $BaseURL URL string to use as BaseURL.
     * @param NULL $BaseURL App::$SiteURL will be used.
     * @param string $Buster Numeric string to use as a cache-buster path segment (first segment).
     * @param NULL $Buster No cache-buster will be used.
     *
     * @see LinkPage::__construct
     *
     * @todo Buster stuff needs work, including properly appending the path segment and perhaps a review
     *       of URL::Set()/Path::Set() stuff.
     */
    public function __construct( \asm\PageSet $PageSet,\asm\App $App,$BaseURL = NULL,$Buster = NULL )
    {
        $this->PageSet = $PageSet;
        $this->App = $App;

        if( !empty($Buster) )
            $this->SetBaseURL($BaseURL,">$Buster");
        else
            $this->SetBaseURL($BaseURL);
    }

    /**
     * cnvyr specific link creation.
     *
     * Bundle example: <?=$lc('css','css-all')?>
     *   File example: <?=$lc('css','style.css')?>
     *
     * @param string $Handler The name of the handler page, typically one of css, img, js or bin.
     * @param string $Filename The filename of the resource or bundle to serve.
     * @retval string The absolute URL of the cnvyr served resource.
     */
    public function __invoke( $Handler = NULL,$Filename = '' )
    {
        $Base = $this->BaseURL;

        if( empty($Handler) || empty($this->PageSet->Pages[$Handler]) )
        {
            Path::Append("PAGE-{$Handler}-NOT-FOUND",$Base['Path']);
            return URL::ToString($Base);
        }

        Path::Merge($this->PageSet->Pages[$Handler]['PathStruct'],$Base['Path']);

//        $Base['Path']['Segments'][] = $Handler;
        $Base['Path']['Segments'][] = $Filename;
        $Base['Path']['IsDir'] = $Base['Path']['IsAbs'] = FALSE;

        return URL::ToString($Base);
    }
}

