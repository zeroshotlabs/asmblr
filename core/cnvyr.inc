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
 * Provide caching and HTTP delivery of assets.
 *
 * This is designed as a base class for functions and should be
 * extended by an application.  Delivery methods can then be
 * defined normally as pages, utilizing base methods of this class.
 *
 * It is recommended that header_remove() be called for all cnvyr methods.
 *
 * All caching is done to a local filesystem path.
 *
 * @todo POssibly add helper methods for HTTP caching, storing cache files, etc.
 * @todo Determine passthru/no-op behavior - i.e. just cache, without image optimization (in helper methods really).
 */
abstract class cnvyr
{
    // Base URL path for nginx's X-Accel-Redirect - needs leading and trailing slashes
    // Must match nginx config (see nginx-asmblr.conf)
    protected static $XAccelRedirectBase = '/xcnvyr/';


    /**
     * Perform a default image serve.
     *
     * The op-token is prepended to the cached resource's filename.
     *
     * This method must be overriden by an application specific page function.
     *
     * @param \asm\App $app Application object.
     * @param string $OpToken Op-token for resource.  Must be unique for different variations of the same resource.
     * @param string $Filename Resource's filename.
     * @param string $OriginBase Absolute origin path of resource.  Use forward slashes.
     *
     * @note The additional parameters are not optional as this is expected to be overridden and
     *       recalled from the parent.
     * @note This could be executed in Load.inc to avoid the asmblr startup (with some config tweaks).
     * @note This exits.
     * @note If dynamic ops are read from the query string, they need to be added to the op-token, otherwise
     *       cache collisions will occur.  This is not done automatically by this method.
     * @todo This may be refactored, specifically to handle ops and split the cache hit with img processing.
     */
    public static function img( \asm\App $app,$OpToken = '',$Filename = '',$OriginFilename = '' )
    {
        $cnvyrCacheDir = $app->Config['cnvyrCacheDir'];
        $cnvyrCacheLocal = $app->Config['cnvyrCacheLocal'];

        // cache directory is relative to app's CacheDir
        if( strpos($cnvyrCacheDir,'/') === FALSE )
            $cnvyrCacheDir = "{$app->CacheDir}/{$cnvyrCacheDir}/";

        // good?
        ini_set('zlib.output_compression',FALSE);
        mb_http_output('pass');

        // first attempt a cache hit if allowed and we're done
        if( $cnvyrCacheLocal === TRUE && static::CacheHit("{$OpToken}_{$Filename}",$cnvyrCacheDir) )
        {
            exit;
        }

        // pull the raw resource, manipulate as needed, and cache if allowed
        // the requested URL is always lowercased thus origin assets (especially on-disk)
        // must also be lowercased
        if( is_readable($OriginFilename) === FALSE )
        {
            HTTP::_404();
            exit;
        }

        $img = new \asm\imgr(file_get_contents($OriginFilename));

        // ops would be performed here
        // TODO: need hooks...?
        // $img->Scale2Width(100);

        $imgbuf = $img->Render();

        if( $cnvyrCacheLocal === TRUE )
        {
            if( empty($OpToken) )
                file_put_contents("{$cnvyrCacheDir}{$Filename}",$imgbuf);
            else
                file_put_contents("{$cnvyrCacheDir}{$OpToken}_{$Filename}",$imgbuf);
        }

        header('Content-Type: '.$img->ContentType);
        echo $imgbuf;

        exit;
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
    protected static function PathParse( $P )
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
            return NULL;
        }

        return array($CacheBuster,$OpToken,$Filename);
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
}





// includes minify
class bundlr
{
}






/**
 * Manipulate images from a buffer string.
 *
 * @note This can make multiple copies of the same image so it'd be slow and memory hungary.  For larger images,
 *       set_time_limit(0) and/or increasing PHP's memory limit may be required.
 *
 * @todo Take a look at further web-optimizations, especially for JPEGS + PNGs.  Right now we're basically
 *       always using truecolor PNGs and only JPEGs get "optimized".
 *
 * @todo Optimize operations such as counting number of colors; determine crop to ratio least-pixels-affected calcs.
 *       http://perplexed.co.uk/1814_png_optimization_with_gd_library.htm
 *       http://stackoverflow.com/questions/3468500/detect-overall-average-color-of-the-picture
 *       http://stackoverflow.com/questions/13642018/optimize-png-when-resizing-with-gd-php-how-to-determine-color-palette
 *
 * @todo Consider adding combo operations, such as CropToRatio: crop width/height to arrive at ratio
 */
class imgr
{
    protected $GDR;

    public $ImageInfo;

    public $ContentType;
    public $Width;
    public $Height;


    /**
     * Create an imgr object from image buffer string.
     *
     * @param string $Buf Image buffer string.,
     * @throws Exception Invalid image string.
     *
     * @note This may be more specific for PNGs.
     */
    public function __construct( $Buf )
    {
        if( ($this->ImageInfo = getimagesizefromstring($Buf)) === FALSE )
            throw new Exception('Invalid image string (info)');

        $this->Width = $this->ImageInfo[0];
        $this->Height = $this->ImageInfo[1];
        $this->ContentType = $this->ImageInfo['mime'];

        if( ($GDRtmp = imagecreatefromstring($Buf)) === FALSE )
            throw new Exception('Invalid image string');

        $this->GDR = imagecreatetruecolor($this->Width,$this->Height);

        // preserve transparency - must be on the destination first - PNGs only...
        imagealphablending($this->GDR,FALSE);
        imagesavealpha($this->GDR,TRUE);

        imagecopy($this->GDR,$GDRtmp,0,0,0,0,$this->Width,$this->Height);

        imagedestroy($GDRtmp);
    }

    /**
     * Render the image and return as a string according to the requested Type.
     *
     * @param string $Type Output format of gif, png or jpeg.  Default uses input format.
     * @throws Exception Unknown output Type.
     * @return string Image buffer.
     *
     * @note For PNGs this forces PNG8 output (256 colors).
     * @note This uses output buffering.
     * @todo We may add option for truecolor PNG, though use JPEGs for now.
     * @todo More optimizations are probably possible, i.e. calc number of colors used
     *       and setting a smaller pallete for PNGs; add compression for JPEGs.
     * @todo GIF isn't very well optimized/supported (i.e. transparency).
     */
    public function Render( $Type = '' )
    {
        if( empty($Type) )
            list($Junk,$Type) = explode('/',$this->ContentType);
        else
            $Type = strtolower($Type);

        ob_start();
        if( $Type === 'gif' )
        {
            imagegif($this->GDR);
        }
        else if( $Type === 'jpeg' )
        {
            imageinterlace($this->GDR,1);
            imagejpeg($this->GDR);
        }
        else if( $Type === 'png' )
        {
//             $GDRtmp = imagecreate($this->Width,$this->Height);
//             imagecopy($GDRtmp,$this->GDR,0,0,0,0,$this->Width,$this->Height);
//             imagepng($GDRtmp,NULL,0,PNG_ALL_FILTERS);
//             imagedestroy($GDRtmp);

            // per class comments, just output truecolor PNGs for now
            imagepng($this->GDR,NULL,9,PNG_ALL_FILTERS);
        }
        else
            throw new Exception("Unknown output Type '{$Type}'");

        return ob_get_contents();
    }

    /**
     * Crop left/right of center to a max pixel count.
     *
     * @param int $Width The new width.
     * @return boolean FALSE if the operation would be a no-op.
     */
    public function Crop2Width( $Width )
    {
        // if the image is less/equal to new width this is a no-op
        if( $this->Width <= $Width )
            return FALSE;

        $Diff = $this->Width - $Width;

        $GDRnew = imagecreatetruecolor($Width,$this->Height);
        imagealphablending($GDRnew,FALSE);
        imagesavealpha($GDRnew,TRUE);

        imagecopyresampled($GDRnew,$this->GDR,0,0,(int)round($Diff/2),0,$Width,$this->Height,$Width,$this->Height);
        imagedestroy($this->GDR);

        $this->GDR = $GDRnew;
        $this->Width = $Width;

        return TRUE;
    }

    /**
     * Crop top/bottom of center to a max pixel count.
     *
     * @param int $Height The new height.
     * @return boolean FALSE if the operation would be a no-op.
     */
    public function Crop2Height( $Height )
    {
        // if the image is less/equal to new height this is a no-op
        if( $this->Height <= $Height )
            return FALSE;

        $Diff = $this->Height - $Height;

        $GDRnew = imagecreatetruecolor($this->Width,$Height);
        imagealphablending($GDRnew,FALSE);
        imagesavealpha($GDRnew,TRUE);

        imagecopyresampled($GDRnew,$this->GDR,0,0,0,(int)round($Diff/2),$this->Width,$Height,$this->Width,$Height);
        imagedestroy($this->GDR);

        $this->GDR = $GDRnew;
        $this->Height = $Height;

        return TRUE;
    }

    /**
     * Scale width down to a max pixel count while maintaining proportions.
     *
     * @param int $Width The new width.
     * @return boolean FALSE if the operation would be a no-op.
     */
    public function Scale2Width( $Width )
    {
        // if the image is less/equal to new width this is a no-op
        if( $this->Width <= $Width )
            return FALSE;

        $Height = (int) round($this->Height*($Width/$this->Width));

        $GDRnew = imagecreatetruecolor($Width,$Height);
        imagealphablending($GDRnew,FALSE);
        imagesavealpha($GDRnew,TRUE);

        imagecopyresampled($GDRnew,$this->GDR,0,0,0,0,$Width,$Height,$this->Width,$this->Height);
        imagedestroy($this->GDR);

        $this->GDR = $GDRnew;
        $this->Width = $Width;
        $this->Height = $Height;

        return TRUE;
    }

    /**
     * Scale height down to a max pixel count while maintaining proportions.
     *
     * @param int $Height The new height.
     * @return boolean FALSE if the operation would be a no-op.
     */
    public function Scale2Height( $Height )
    {
        // if the image is less/equal to new width this is a no-op
        if( $this->Height <= $Height )
            return FALSE;

        $Width = (int) round($this->Width*($Height/$this->Height));

        $GDRnew = imagecreatetruecolor($Width,$Height);
        imagealphablending($GDRnew,FALSE);
        imagesavealpha($GDRnew,TRUE);

        imagecopyresampled($GDRnew,$this->GDR,0,0,0,0,$Width,$Height,$this->Width,$this->Height);
        imagedestroy($this->GDR);

        $this->GDR = $GDRnew;
        $this->Width = $Width;
        $this->Height = $Height;

        return TRUE;
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

