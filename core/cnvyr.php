<?php
/**
 * @file cnvyr.php cnvyr.io API client and page functions.
 * @author Stackware, LLC
 * @version 5.0
 * @copyright Copyright (c) 2012-2023 Stackware, LLC. All Rights Reserved.
 * @copyright Licensed under the GNU General Public License
 * @copyright See COPYRIGHT.txt and LICENSE.txt.
 */
namespace asm;


/**
 * cnvyr serve (cnvyrsrv) provides pre-assembled page functions for operating on assests using the
 * <a href="http://cnvyr.io/">cnvyr.io online minifying/thumbnailer API</a> and delivering the result.
 *
 * cnvyrsrv::FromCache can be called statically in an application's Load.inc which will
 * skip the rest of asmblr execution.  cnvyrsrv::FromCache will use web-server optimized delivery
 * i.e. X-Accel-Redirect or X-sendfile, when available.
 *
 * cnvyrsrv serves two types of files by default, each with it's own method:
 *  - cnvyrsrv::text - TemplateSet assets, such as CSS and Javascript files.
 *  - cnvyrsrv::binary - Image or other binary files.  PDFs, docs, fonts, etc. are only cached, with no operations performed.
 *
 * cnvyrsrv also supports bundles of CSS or Javascript files which combine multiple files into one and perform operations
 * on the result (asset pipeline).  Bundles are defined in the stub class using the cnvyrsrv::$Bundles property
 * with the following structure:
 *  @code
 *  public static $Bundles = array('bundle.css'=>
 *                            array(array('css_bootstrap.min','css_jquery-ui-1.10.3.custom','css_style'),array('min'=>'css','gzip'=>'1')),
 *                                  'bundle.js'=>
 *                            array(array('js_jquery-1.10.2.min','js_jquery-ui-1.10.3.custom.min','js_bootstrap.min','js_sitelib'),array('min'=>'js','gzip'=>'1')));
 *  @endcode
 *
 * A bundle is referenced by it's key, ie. @c bundle.css or @c bundle.js in the example above.  Each filepart
 * of a bundle is a template token.  Not that bundles can and should contain their own ops to perform on the bundled asset.
 */
abstract class cnvyrsrv
{
    /**
     * @var string $XAccelRedirectBase
     * Base URL for serving via nginx's X-Accel-Redirect.  Needs leading and trailing
     * slashes and must match nginx config (see nginx-asmblr.conf).
     *
     * Override this property (though rarely needed).
     */
    protected static $XAccelRedirectBase = '/xcnvyr/';

    /**
     * @var array $Bundles
     * Definition of CSS or JS bundles.
     *
     * Override this property to define bundles.
     */
    public static $Bundles = array();


    /**
     * @var array $Ops
     * Key/value pairs of cnvyr API operations to perform.
     *
     * @see ResolveRequest
     */
    protected static $Ops = array();

    /**
     * @var string $OriginDir
     * The local directory relative to App::$AppRoot that contains the asset.
     *
     * By default this is the second to last path segment of the request URL.
     *
     * @see ResolveRequest
     */
    protected static $OriginDir = '';

    /**
     * @var string $Filename
     * The filename of the asset within $OriginDir.
     *
     * By default this is the last path segment of the request URL.
     *
     * @see ResolveRequest
     */
    protected static $Filename = '';

    /**
     * @var string $ContentType
     * The content type of the asset.
     *
     * By default this is the extension of the filename.  It'll be normalized to a full MIME
     * content-type using HTTP::ContentType().
     *
     * @see ResolveRequest
     */
    protected static $ContentType = '';

    /**
     * @var string $CachePrefix
     * The prefix to prepend to $Filename to create the cached file.
     *
     * By default $OriginDir is used.  cnvyrPrefix from the manifest is always prepended to this.
     *
     * @see ResolveRequest
     */
    protected static $CachePrefix = '';

    /**
     * @var asm::TemplateSet $TemplateSet
     * For text/template assets, the TemplateSet used to render the templates.
     *
     * @see ResolveRequest
     */
    protected static $TemplateSet = NULL;

    /**
     * @var string $TemplateToken
     * For text/template assets, the token of the template to render.
     *
     * By default this is the filename without and extension and prefixed with OriginDir and an underscore.  For
     * example, a request of @c /js/main.js will result in the template token @c js_main
     *
     * @see ResolveRequest
     */
    protected static $TemplateToken = '';


    /**
     * Attempt a local cache hit and serve the cached resource if available.
     *
     * This is typically called in an application's Load.inc which will skip the rest of
     * asmblr execution for a very fast response.
     *
     * In order to optimize the delivery of cached files, the asset's URL is expected to map
     * directly to the filename with a $CachePrefix.  Thus a request for @c /css/style.css is
     * cached as:
     * @code
     *  xyz_css_style.css
     * @endcode
     *
     * where @c xyz is the manifest set cnvyrPrefix config directive and @c css is the CachePrefix.
     * Thus, the page URL's second to last path segment and CachePrefix should be the same.
     *
     * @param string $Filename The filename of the cached resource to be checked for and served.
     * @param string $CacheDir The full path to the cache directory for cnvyr with trailing slash.
     * @param boolean $DirectOut TRUE to avoid using any server optimization (i.e. X-SendFile).
     * @retval boolean TRUE if the cache hit was successfully served, FALSE if it didn't exist.
     *
     * @note $Filename and $CacheDir are trusted - do checks elsewhere.
     * @note Serving gzip'd cache files is only supported using nginx - with Apache's mod_xsendfile
     *       Content-Encoding is stripped and won't work.
     * @note Images and other binary assets aren't gzip'd by default.
     * @note Since asmblr doesn't lowercase the request URL by default, this method will be cache sensitive
     *       when operating on a case-sensitive filesystem, i.e. Linux.
     * @note The cached filename should contain an accurate extension - otherwise a content type header
     *       will have to be sent explicitly in most cases.
     */
    public static function FromCache( $Filename,$CacheDir,$DirectOut = FALSE,$HTTPCacheTime = '3600' )
    {
        if( is_readable($CacheDir.$Filename) === FALSE )
            return FALSE;

        // set the HTTP cache time if not empty
        if( !empty($HTTPCacheTime) )
            HTTP::Cache($HTTPCacheTime);

        // explicitly set the content-type because it's too unpredictable between web servers/environments
        HTTP::ContentType(HTTP::Filename2ContentType($Filename));

        if( $DirectOut )
        {
            header('Content-Length: '.filesize($CacheDir.$Filename));
            readfile($CacheDir.$Filename);
        }
        // nginx
        else if( PHP_SAPI === 'fpm-fcgi' )
        {
            // nginx generally sets correct etag/last-modified/content-length headers based on the cached file
            // it seems that nginx will end the request when serving an image, but not CSS or Javascript
            header('X-Accel-Redirect: '.static::$XAccelRedirectBase.$Filename);
        }
        // apache2 with mod_xsendfile
        else if( PHP_SAPI === 'apache2handler' )
        {
            // this should automatically set correct content-length, etag, vary, etc.
            header("X-SendFile: {$CacheDir}{$Filename}");
        }
        // everything else - same as when DirectOut is TRUE
        else
        {
            header('Content-Length: '.filesize($CacheDir.$Filename));
            readfile($CacheDir.$Filename);
        }

        return TRUE;
    }


    /**
     * Perform operations and serve a single or bundle of text templates.
     *
     * This is used for serving CSS and Javascript templates, which are rendered using the provided TemplateSet.
     *
     * An application's stub cnvyrsrv should override this method which is then set as a page function, typically:
     * @code
     *  css  /css/  Active  cnvyrsrv::css
     * @endcode
     *
     * The stub method itself provides the ops to perform, such as:
     * @code
     *  public static function css( $app )
     *  {
     *      parent::text($app,array('min'=>'css','gzip'=>'1'));
     *  }
     * @endcode
     *
     * The URL of this page is expected to be /css/ because no $CachePrefix is set - i.e. the second to last
     * segment of the request URL, the OriginDir (templates/css/) and the CachePrefix are all expected to be the same.
     *
     * If no $Ops are set, no API calls will be performed and the asset will only be cached.
     *
     * @param asm::App $app The application's app object.
     * @param array $Ops The operations to perform on the asset or bundle.
     * @param string $OriginDir The directory containing the asset, relative to the TemplateSet's base directory.
     * @param string $CachePrefix The prefix to prepend to the cached file.
     * @param string $Filename The filename of the bundle or file.
     * @param string $ContentType The content type of the bundle or file.
     * @param asm::TemplateSet $TemplateSet The TemplateSet containing the asset(s).
     *
     * @note Since asmblr doesn't lowercase the request URL by default, this method will be cache sensitive
     *       when operating on a case-sensitive filesystem, i.e. Linux.
     * @note Passhthru behavior (an empty cnvyrAPIURL) is implemented here.
     *
     * @see ResolveRequest for customizing parameters.
     */
    protected static function text( \asm\App $app,$Ops,$OriginDir = '',$CachePrefix = '',$Filename = '',$ContentType = '',\asm\TemplateSet $TemplateSet = NULL )
    {
        static::ResolveRequest($app,$Ops,$OriginDir,$CachePrefix,$Filename,$ContentType,$TemplateSet);

        header_remove();

        $cc = new cnvyrc($app);

        // we have a bundle - a bundle's ops will be overwritten if they're passed as an argument
        if( ($Payload = static::ResolveBundle(static::$Filename,static::$TemplateSet)) !== NULL )
        {
            // passthru mode - simply concat each file
            if( $cc->PassthruMode === TRUE )
            {
                $Payload = implode("\n\n",$Payload[0]);
            }
            else
            {
                if( !empty(static::$Ops) )
                    $Payload[1] = static::$Ops;

                if( !empty($Payload[1]) )
                    $Payload = $cc->ToAPI($Payload[0],$Payload[1]);
                else
                    $Payload = implode("\n\n",$Payload[0]);
            }
        }
        // no bundle - treat as a single template
        else
        {
            $Payload = static::$TemplateSet->Render(static::$TemplateToken);

            if( $cc->PassthruMode === FALSE )
            {

                if( !empty(static::$Ops) )
                    $Payload = $cc->ToAPI($Payload,static::$Ops);
            }
        }

        if( empty($Payload) )
        {
            HTTP::_404();
            exit;
        }

        // will auto-cache if config allows
        if( $cc->PassthruMode === FALSE )
            $cc->ToCache($app,$Payload,static::$Filename);
        else
            static::$Ops['gzip'] = FALSE;

        static::srvout($Payload,static::$ContentType,empty(static::$Ops['gzip'])?FALSE:TRUE);
    }


    /**
     * Perform operations and serve a binary asset.
     *
     * This is used for serving images, as well as other binary files such as .doc/.pdf/etc and fonts.
     *
     * An application's stub cnvyrsrv should override this method which is then set as a page function, typically:
     * @code
     *  img  /img/  Active  cnvyrsrv::img
     * @endcode
     *
     * The stub method itself provides the ops to perform, such as:
     * @code
     * public static function img( $app )
     * {
     *     parent::binary($app,array('opt'=>'1'),'media','img');
     * }
     * @endcode
     *
     * Assets will be served from @c {AppRoot}/media with a URL base of @c /img/
     *
     * Here is an example that automatically creates thumbnails of the same images, but using different cache prefix
     * and base URL.  A unique cache prefix is required so that images of different dimensions don't overwrite each other.
     *
     * @code
     *  imgthumb  /imgthumb/  Active  cnvyrsrv::imgthumb
     * @endcode
     *
     * @code
     * public static function imgthumb( $app )
     * {
     *     parent::binary($app,array('sw2tn'=>'200','opt'=>'1'),'media','imgthumb');
     * }
     * @endcode
     *
     * This will scale to a 200px wide optimized thumbnail.  Assets are served from the @c media directory, but are
     * prefixed with @c imgthumb when cached, which also matches the request's URL base path.
     *
     * @param asm::App $app The application's app object.
     * @param array $Ops The operations to perform on the asset or bundle.
     * @param string $OriginDir The directory containing the asset, relative to App::AppRoot.
     * @param string $CachePrefix The prefix to prepend to the cached file.
     * @param string $Filename The filename of the bundle or file.
     * @param string $ContentType The content type of the bundle or file.
     * @param string $Payload The buffer containing the asset.
     *
     * @note Since asmblr doesn't lowercase the request URL by default, this method will be cache sensitive
     *       when operating on a case-sensitive filesystem, i.e. Linux.
     * @note Passhthru behavior (an empty cnvyrAPIURL) is implemented here.
     *
     * @see ResolveRequest for customizing parameters.
     */
    protected static function binary( \asm\App $app,$Ops,$OriginDir = '',$CachePrefix = '',$Filename = '',$ContentType = '',$Payload = '' )
    {
        static::ResolveRequest($app,$Ops,$OriginDir,$CachePrefix,$Filename,$ContentType);

        header_remove();

        $cc = new cnvyrc($app);

        $Origin = "{$app->AppRoot}/".static::$OriginDir.'/';

        if( empty($Payload) )
        {
            if( is_readable($Origin.static::$Filename) )
            {
                $Payload = file_get_contents($Origin.static::$Filename);
            }
        }

        if( empty($Payload) )
        {
            HTTP::_404();
            exit;
        }

        if( $cc->PassthruMode === FALSE )
        {
            if( !empty(static::$Ops) )
                $Payload = $cc->ToAPI($Payload,static::$Ops);

            // will cache if config allows
            $cc->ToCache($app,$Payload,static::$Filename);
        }
        else
            static::$Ops['gzip'] = FALSE;

        static::srvout($Payload,static::$ContentType,empty(static::$Ops['gzip'])?FALSE:TRUE);
    }


    /**
     * Directly output payload with proper headers.
     *
     * @param string $Payload The buffer to output.
     * @param string $ContentType MIME content type header.
     * @param boolean $gzip Boolean to set content encoding.
     *
     * @note This method exits.
     */
    protected static function srvout( $Payload,$ContentType,$gzip )
    {
        header('Content-Length: '.strlen($Payload));
        HTTP::ContentType($ContentType);

        if( $gzip === TRUE )
            header('Content-Encoding: gzip');

        echo $Payload;
        exit;
    }


    /**
     * Determine parameters for serving an asset based on provided values and the default request.
     *
     * This helper method merges request parameters with manually provided ones, typically from a stub class's
     * page function.  This will set the class's static properties which are then used in methods such as
     * cnvyrsrv::text() and cnvyrsrv::binary().
     *
     * The logic for resolution:
     *
     *  - @c Ops: The operations to perform on the asset - must be set explicitly, otherwise no API call will be performed (only caching).
     *  - @c OriginDir: The directory containing the asset; the second to last path segment of the request URL is used by default.
     *  - @c CachePrefix: The cache filename's prefix which should be unique per asset type.  Defaults to $OriginDir.
     *  - @c Filename: The filename of the asset.  Note that the extension is used to determine the content type by default.
     *  - @c ContentType: The content type of the asset.  By default the extension of $Filename is used.
     *  - @c TemplateSet: The TemplateSet containing the text templates (CSS/JS) to render, if applicable.
     *
     * @param asm::App $app The application's app object.
     * @param array $Ops The operations to perform on the asset or bundle.
     * @param string $OriginDir The directory containing the asset, relative to the TemplateSet's base directory.
     * @param string $CachePrefix The prefix to prepend to the cached file.
     * @param string $Filename The filename of the bundle or file.
     * @param string $ContentType The content type of the bundle or file.
     * @param asm::TemplateSet $TemplateSet The TemplateSet containing the asset(s).
     *
     * @see cnvyrsrv::text()
     * @see cnvyrsrv::binary()
     *
     * @todo Some of this (Filename parse) is redundant when a payload is provided by a caller.  Content-Type may also need to be provided when Filename is not.
     */
    protected static function ResolveRequest( \asm\App $app,$Ops = array(),$OriginDir = '',$CachePrefix = '',$Filename = '',$ContentType = '',\asm\TemplateSet $TemplateSet = NULL )
    {
        if( empty($Filename) )
            static::$Filename = \asm\Path::Bottom($app->Request['MatchPath']);
        else
            static::$Filename = $Filename;

        $FNPI = pathinfo(static::$Filename);

        if( empty($ContentType) )
        {
            if( empty($FNPI['extension']) )
                llog('Couldn\'t determine extension/content-type for \''.static::$Filename).'\'';
            else
                static::$ContentType = $FNPI['extension'];
        }
        else
            static::$ContentType = $ContentType;

        // there are no default ops - always set by the method, otherwise will be a no-op by default
        if( !empty($Ops) )
            static::$Ops = $Ops;

        if( empty($TemplateSet) )
            static::$TemplateSet = $app->html;
        else
            static::$TemplateSet = $TemplateSet;

        if( empty($OriginDir) )
            static::$OriginDir = \asm\Path::Get(-2,$app->Request['MatchPath']);
        else
            static::$OriginDir = $OriginDir;

        static::$TemplateToken = static::$OriginDir.'_'.pathinfo(static::$Filename)['filename'];

        if( empty($CachePrefix) )
            static::$CachePrefix = $app->Config['cnvyrPrefix'].'_'.static::$OriginDir;
        else
            static::$CachePrefix = $app->Config['cnvyrPrefix'].'_'.$CachePrefix;
    }


    /**
     * Resolve a filename to a bundled payload rendered by a TemplateSet.
     *
     * $Filename is the requested filename of pre-defined bundle.  See class documentation
     * for an example of defining a bundle.
     *
     * @param string $Filename Bundle filename.
     * @param \asm::TemplateSet $TS The TemplateSet containing the templates to render.
     * @retval array A two part array containing an array of payload strings and ops as the second element.
     * @retval NULL The bundle couldn't be resolved.
     */
    protected static function ResolveBundle( $Filename,\asm\TemplateSet $TS )
    {
        if( isset(static::$Bundles[$Filename]) )
        {
            $Payload = array();
            foreach( static::$Bundles[$Filename][0] as $K => $F )
            {
                if( isset($TS->$F) )
                {
                    $Payload[$Filename.$K] = $TS->Render($F);
                }
                else
                {
                    llog("Template {$F} of bundle {$Filename} not found");
                    return NULL;
                }
            }

            // Ops from bundle definition
            $Ops = empty(static::$Bundles[$Filename][1])?array():static::$Bundles[$Filename][1];

            return array($Payload,$Ops);
        }
        else
            return NULL;
    }
}



/**
 * cnvyr client (cnvyrc) provides connectivity and result caching for the
 * <a href="http://cnvyr.io">cnvyr.io minify thumbnail API</a>.
 *
 * The following configuration directives must be set in the manifest's Config tab.
 *
 *  - @c cnvyrAPIURL: full URL of cnvyr.io API (typically http://srv.cnvyr.io/v1) or empty for passthru behavior
 *  - @c cnvyrCacheLocal: boolean TRUE or FALSE whether to cache results from cnvyr.io
 *  - @c cnvyrCacheDir: local filesystem directory to cache assests if above is TRUE.  Must be writeable by the web processes.
 *  - @c cnvyrPrefix: A short string to prefix each cached resource.  Must be unique per app.
 *
 * All caching is done to a local filesystem path.
 *
 * @note If cnvyrAPIURL is empty, passhthru behavior will be active and automatically disable caching and gzip.  Passthru behavior is actually
 *       implemented by cnvyrsrv::text() and cnvyrsrv::binary().  Bundled files are concatenated together.
 * @note Pre-gzip'd cached assets (those gzip'd by the API) and using x-sendfile will only work with nginx.
 * @note Cached assets must have the correct file extension in order to have the content type set.
 * @note If @c cnvyrCacheDir is set to a path in /tmp the following may be useful to avoid automatic deletion of
 *       cached assets.  Add the following flag to the first call of tmpwatch, generally in /etc/cron.daily/tmpwatch right before the 10d
 *       @code
 *       -x /tmp/cnvyr-cache
 *       @endcode
 */
class cnvyrc extends restr
{
    /**
     * @var boolean $PassthruMode
     * TRUE if an empty cnvyrAPIURL has been set.  Used by cnvyrsrv::text() and cnvyrsrv::binary().
     */
    public $PassthruMode = FALSE;

    /**
     * @var array $Config
     * Configuration variables from the manifest.
     */
    protected $Config = array();


    /**
     * Instantiate a cnvyrc object for API communication and caching of the result.
     *
     * An empty cnvyrAPIURL will put the object in passthru mode and disable caching and gzip.
     *
     * @param App $app The application's App object.
     */
    public function __construct( $app )
    {
        $this->Config = $app->Config;

        if( empty($this->Config['cnvyrAPIURL']) )
            $this->PassthruMode = TRUE;
        else
            parent::__construct($this->Config['cnvyrAPIURL']);
    }

    /**
     * Perform an API request with one or more buckets of content.
     *
     * Each bucket of content is a string of data to send to the API.  For CSS/JS,
     * an array of strings is acceptable as a bundle.  For images, only a single bucket
     * is accepted.
     *
     * This will trigger a HTTP 500 error and exit if the API call fails.
     *
     * @param string $Payload A string that's the payload to operate on.
     * @param array $Payload An array of strings to bundle and perform operations on (CSS/JS only).
     * @param array $Ops Associative array of ops for the API to perform.
     * @retval string The processed result from the cnvyr.io API.
     */
    public function ToAPI( $Payload,$Ops )
    {
        // treat as a bundle
        if( is_array($Payload) === TRUE )
        {
            $Bundle = array();
            $i = 0;
            foreach( $Payload as $K => $V )
            {
                $Bundle['@files'.$i] = $V;
                ++$i;
            }

            $Response = $this->POSTFiles('',$Ops,array(),$Bundle);
        }
        // single
        else
        {
            $Response = $this->POSTFiles('',$Ops,array(),array('@files0'=>$Payload));
        }

        if( !empty($this->CURLError) )
        {
            llog($this->CURLError);
            llog($Response);
            HTTP::_500();
        }

        return $Response;
    }


    /**
     * Cache a payload.
     *
     * The payload can be any string, typically a result from the API, or a string that
     * doesn't require any processing (PDF, fonts, doc, etc).  Cached payloads can then
     * be served by FromCache().
     *
     * @param string $Payload The payload to cache.
     * @param string $Prefix The prefix to prepend to filename, usually from the manifest's config.
     * @param string $Filename The cache filename.
     * @retval int The number of bytes written to cache.
     * @retval boolean FALSE if the save failed.
     * @retval NULL The system is configured not to cache - nothing was cached.
     *
     * @note Existing cache files are silently overwritten.  There is no locking.
     * @note The cached filename should contain an accurate extension - otherwise a content type header
     *       will have to be sent explicitly upon serving.
     *
     * @see cnvyrsrv::FromCache()
     */
    public function ToCache( \asm\App $app,$Payload,$Filename )
    {
        if( !empty($this->Config['cnvyrCacheLocal']) )
        {
            $CacheFile = $app->Config['cnvyrPrefix'].'_'.implode('_',$app->Request['MatchPath']['Segments']);
//            $CacheFile = $app->Config['cnvyrPrefix'].'_'.$app->Request['MatchPath']['Segments'][1].'_'.$app->Request['MatchPath']['Segments'][2]
//            $CacheFile = $Prefix.'_'.$Filename;

            return file_put_contents($this->Config['cnvyrCacheDir'].'/'.$CacheFile,$Payload);
        }
        else
            return NULL;
    }


    /**
     * Clears the cached assets for the app (those matching the manifest's @c cnvyrPrefix).
     */
    public function ClearCache()
    {
        $F = glob("{$this->Config['cnvyrCacheDir']}{$this->Config['cnvyrPrefix']}_*");

        foreach( $F as $K => $V )
            unlink($V);

        return $F;
    }
}



/**
 * Create URLs for cnvyrc managed assets.
 *
 * @note Largely similar to LinkPage.  Modifications to __invoke().
 */
class Linkcnvyr extends LinkPage
{
    /**
     * Linkcnvyr constructor.
     *
     * @param PageSet $PageSet PageSet containing Pages to create URLs for.
     * @param App $App The application's App object.
     * @param array $BaseURL URL Struct to use as BaseURL.
     * @param string $BaseURL URL string to use as BaseURL.
     * @param NULL $BaseURL App::$SiteURL will be used.
     * @param string $Buster Numeric string to use as a cache-buster path segment (first segment).
     * @param NULL $Buster No cache-buster will be used.
     *
     * @todo Buster stuff needs work, including properly appending the path segment and perhaps a review
     *       of URL::Set() and Path::Set() stuff.
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
     * @param array $Ops Op => value array of cnvyr ops to set for the URL.
     * @retval string The absolute URL of the cnvyr served resource.
     *
     * @note $Ops will be constructed as an encoded 'c' query parameter.  Some installs of asmblr may not support custom $Ops.
     */
    public function __invoke( $Handler = NULL,$Filename = '',$Ops = array() )
    {
        $Base = $this->BaseURL;

        if( empty($Handler) || empty($this->PageSet->Pages[$Handler]) )
        {
            Path::Append("HANDLER-{$Handler}-NOT-FOUND",$Base['Path']);
            return URL::ToString($Base);
        }

        Path::Merge($this->PageSet->Pages[$Handler]['PathStruct'],$Base['Path']);

//        $Base['Path']['Segments'][] = $Handler;
        $Base['Path']['Segments'][] = $Filename;
        $Base['Path']['IsDir'] = $Base['Path']['IsAbs'] = FALSE;

        if( !empty($Ops) )
            URL::Set(array('c'=>http_build_query($Ops)),$Base);

        return URL::ToString($Base);
    }
}

