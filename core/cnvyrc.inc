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
 * @todo Possibly add helper methods for HTTP caching, storing cache files, etc.
 * @todo Determine passthru/no-op behavior - i.e. just cache so it can be sent via x-sendfile.
 * @todo And passthru/proxy behavior to avoid CORS issues. perhaps this could even be a paid serrvicee.
 */
abstract class cnvyrc extends restr
{
    // configuration required by extending class
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
     * Serve an asset.
     *
     * The op-token is prepended to the cached resource's filename.
     *
     * @param string $OpToken Op-token for resource.  Must be unique for different variations of the same resource.
     * @param string $Filename Resource's filename.
     * @param array $Ops Associative array of ops.
     * @param string $Content File content to send as $Filename.
     *
     * @note This could be executed in Load.inc to avoid the asmblr startup (with some config tweaks).
     * @note This exits.
     * @note If dynamic ops are read from the query string, they need to be added to the op-token, otherwise
     *       cache collisions will occur.  This is not done automatically by this method.
     * @todo Needs some way to define image ops/etc
     */
    public function srv( $OpToken,$Filename,$Ops = array(),$Content = '' )
    {
        if( !empty($OpToken) )
            $OpToken = $OpToken.'_';

        // first attempt a local cache hit if allowed and we're done
        if( $this->CacheLocal === TRUE && static::CacheHit($OpToken.$Filename,$this->CacheDir) )
        {
            exit;
        }

        // pull the raw resource and cache if allowed

        // the requested URL is always lowercased thus origin assets (especially on-disk)
        // must also be lowercased
        if( is_readable($this->OriginDir.$Filename) === FALSE )
        {
            HTTP::_404();
            exit;
        }

        // TODO: probably need some error checking
        if( empty($Content) )
            $Response = $this->POSTFiles('v1',$Ops,array(),array('files0'=>$this->OriginDir.$Filename));
        else
            $Response = $this->POSTFiles('v1',$Ops,array(),array('@files0'=>$this->OriginDir.$Filename));

        if( $this->CacheLocal === TRUE )
            file_put_contents($this->CacheDir.$OpToken.$Filename,$Response);

        header('Content-Type: '.$this->Headers['content-type']);
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
     *       the rest of asmblr execution.  However the caller typically depends on Config,
     *       values, at this point anyway.
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
            // required so that nginx sets the correct header.
            // nginx generally also sets correct etag/last-modified headers based on the cache file.
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
        // everything else IIS (cgi-fcgi)/GAE/etc
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
     * Parse a path into filename and op-token components.
     *
     * This requires a Path Struct, typically the request's MatchPath, and expects a path in the form:
     *   /any-page-url/{optional-cache-buster}/{optional-op-token}/filename.ext
     *
     * A cache-buster must be all digits and is generally ignored.
     *
     * The resource filename and op-token are rawurldecode()'d and stripped of '..' and '/' characters.
     *
     * @param array $P Path Struct to parse.
     * @retval array Numeric array with cache-buster, op-token and filename elements.
     * @retval NULL The path could not be parsed.
     */
    public static function PathParse( $P )
    {
        // /anything/cache-buster/op-token pattern
        if( !empty($P['Segments'][3]) )
        {
            $CacheBuster = $P['Segments'][1];
            $OpToken = str_replace(array('..','/'),'',rawurldecode($P['Segments'][2]));
            $Filename = str_replace(array('..','/'),'',rawurldecode($P['Segments'][3]));
        }
        // a cache-buster or op-token depending on whether the segment is all digits.
        else if( !empty($P['Segments'][2]) )
        {
            $Filename = str_replace(array('..','/'),'',rawurldecode($P['Segments'][2]));

            if( ctype_digit($P['Segments'][1]) === FALSE )
            {
                $OpToken = str_replace(array('..','/'),'',rawurldecode($P['Segments'][1]));
                $CacheBuster = '';
            }
            else
            {
                $OpToken = '';
                $CacheBuster = $P['Segments'][1];
            }
        }
        // just a filename
        else if( !empty($P['Segments'][1]) )
        {
            $Filename = str_replace(array('..','/'),'',rawurldecode($P['Segments'][1]));
            $OpToken = $CacheBuster = '';
        }
        // nothing (probably a 404)
        else
        {
            return array();
        }

        return array($CacheBuster,$OpToken,$Filename);
    }
}




// handy for applying changes - may be good to extend to LinkSet related stuff
// if( $K === 'CropWH' )
// {
//     $WH = explode('x',$V);

//     if( count($WH) !== 2 || !ctype_digit($WH[0]) || !ctype_digit($WH[1]) )
//         throw new Exception('Invalid CropWH parameters.');

//     $this->CropWH((int)$WH[0],(int)$WH[1]);
// }
// else if( $K === 'MaxW' || $K === 'MaxH' )
// {
//     if( strpos($V,'&scale') !== FALSE )
//         $this->{$K}((int)str_replace('&scale','',$V),TRUE);
//     else
//         $this->{$K}((int)$V,FALSE);
// }

// suited for prepending path segments to a filename
// somehow add cache breaking URLs (would need prefix segment that cnvyr knows to strip out)
class Linkcnvyr extends LinkSet
{
    // call as $limg('Filename.jpg','semgnet1/segmentN');
    public function __invoke( $Name = NULL,$Set = '' )
    {
        $Base = $this->BaseURL;
        if( !empty($Set) )
        {
            foreach( explode('/',$Set) as $S )
                $Base['Path']['Segments'][] = $S;
        }

        $Base['Path']['Segments'][] = $Name;
        $Base['Path']['IsDir'] = $Base['Path']['IsAbs'] = FALSE;

        return URL::ToString($Base);
    }
}

