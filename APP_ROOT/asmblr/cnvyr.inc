<?php
namespace asm;


// this doesn't use a database
class cnvyr extends \fw\WireableBase
{
    // meta data for non-cached requests
    protected $Origin = '';

    protected $LocalCache = '';

    // stuff for SrvImg
    protected $GD;
    protected $GDW = 0;
    protected $GDH = 0;
    protected $GDColorCount = 0;


    // local cache must have a trailing slash and already exist/writeable by the web server
    public function __construct( $LocalCache )
    {
        $this->LocalCache = $LocalCache;
    }

    // serve a path based on a config - Token is used for cache files
    // a config must minimally contain an Origin. the origin must currently
    // be a local file or http URL or ContentSet object
    // this exits - could probably be abused - this whole class could probably be static
    public function Srv( $Path,$Token,$Config )
    {
        // first check if we can send a cached copy - this could be a security concern - openbasedir/etc?
        if( $Path{0} === '/' || $Path{0} === '.' || empty($Path) )
            throw new Exception("Invalid Path '{$Path}'");

        header_remove();

        // ties us to nginx for now
        // cached files are stored in a single directory, each suffixed by Token
        // Srv() could also be called in asmboot.php under it's own domain to bypass almost all of asmblr startup
        // do we need any config awareness here, such as cache?  \fw\HTTP::Cache($V[1]);
        // TODO: /cnvyri/ should be configurable
        // we should probably do some path normalization (replace slashes... or md5?)
        if( is_file("{$this->LocalCache}{$Token}_{$Path}") && $Config['NoLocalCache'] === FALSE )
        {
            if( PHP_SAPI === 'apache2handler' )
            {
                // this dance sucks - can't mod_xsendfile send the Content Type ?
                $finfo = finfo_open(FILEINFO_MIME);
                header('Content-Type: '.finfo_file($finfo,"{$this->LocalCache}{$Token}_{$Path}"));
                header("X-SendFile: {$this->LocalCache}{$Token}_{$Path}");

            }
            else
                header("X-Accel-Redirect: /cnvyri/{$Token}_{$Path}");

            exit;
        }

        if( empty($Config['Origin']) )
            throw new Exception("No Origin for {$Path}");

        // do we need to normalize Origin if a string?
        // security was already checked and all complex read checking :)
        if( $Config['Origin'] instanceof \asm\ContentSet )
        {
            // basically same as Page SrvImg
            $C = $Config['Origin']->Match(asm()->SrvSite['_id'],$Path);
            if( empty($C) )
            {
                \fw\HTTP::_404();
            }
            else
            {
                if( $C['Status'] !== 'Active' )
                {
                    \fw\HTTP::_404();
                }
                else
                {
                    // since an image, always the first case, right?
                    if( is_object($C['Body']) )
                        $Buf = $C['Body']->bin;
                    else
                        $Buf = $C['Body'];
                }
            }
        }
        else
        {
            $Buf = @file_get_contents($Config['Origin'].$Path);

            if( strpos($Config['Origin'],'http') === 0 )
            {
                // blind assumption
                $HTR = $http_response_header;

                if( empty($HTR[0]) || strpos($HTR[0],'200') === FALSE )
                {
                    \fw\HTTP::_404();
                    exit;
                }
            }
        }

        if( empty($Buf) )
        {
            \fw\HTTP::_404();
            exit;
        }

        $FI = new \finfo(FILEINFO_MIME_TYPE);
        $OriginContentType = explode('/',$FI->buffer($Buf));

        // NOTE: Config may be altered by the op handler
        if( $OriginContentType[0] === 'image' )
        {
            $OutputBuf = $this->SrvImg($Buf,$Config,$OriginContentType);
        }
        else
        {
            throw new Exception("Unknown Origin ContentType '{$OriginContentType[0]}'");
        }

        // we would handle non-op handler config, like Cache (which is HTTP caching)
        // perhaps we don't even do cache since it can be easily done by the calling Page or nginx

        // write to local cache and serve using x-sendfile
        file_put_contents("{$this->LocalCache}{$Token}_{$Path}",$OutputBuf);

        if( PHP_SAPI === 'apache2handler' )
        {
            // this dance sucks - can't mod_xsendfile send the Content Type ?
            $finfo = finfo_open(FILEINFO_MIME);
            header('Content-Type: '.finfo_file($finfo,"{$this->LocalCache}{$Token}_{$Path}"));
            header("X-SendFile: {$this->LocalCache}{$Token}_{$Path}");
        }
        else
            header("X-Accel-Redirect: /cnvyri/{$Token}_{$Path}");

        exit;
    }

    // perform one or more change operations (based on query strings passed as configs)
    // optimize output image type/resolution/etc
    // return output as buffer
    // we only pay attention to Config directives/ops that we understand - the rest are expected to be handled elsewhere
    // for big images like http://cms.cb.asmsrv.co//cnvyr/cprelated/190x190/2012_09-12-12_ferrara_snow-white-wine-bar.jpg PHP's mem
    // limit needs to be bumped up (or perhaps the below could be optimized) - plus set_time_limit(0)
    protected function SrvImg( $Buf,&$Config,$OriginContentType )
    {
        $this->GD = imagecreatefromstring($Buf);
        $this->GDW = imagesx($this->GD);
        $this->GDH = imagesy($this->GD);
        $this->GDColorCount = imagecolorstotal($this->GD);

        // automatically optimize ContentType or use configured value, which must be a commong image ext
        // this still isn't bulletproof in which case specify png in the config
        // http://www.carcarekiosk.com/img/parts-and-tools/battery-chargers.png
        if( empty($Config['ContentType']) || $Config['ContentType'] === 'optimize' )
        {
            // sometimes an image will be truecolor but only actually use a small number
            // of colors - detect that, inefficiently, by creating a palette image,
            // copying, and then determining how many colors end up in the palette
            // note that creating a truecolor then converting it to palette makes a messy image
            // (says there are for example 28 colors vs actually 173)
            $GDtmp = imagecreate($this->GDW,$this->GDH);

//             imagecolortransparent($GDtmp,imagecolorallocatealpha($GDtmp,0,0,0,127));
//             imagealphablending($GDtmp,FALSE);
//             imagesavealpha($GDtmp,TRUE);

            imagecopy($GDtmp,$this->GD,0,0,0,0,$this->GDW,$this->GDH);

            // if we're at the threshold we assume it's true truecolor (photo) and will use a jpeg
            if( ($CNT = imagecolorstotal($GDtmp)) === 256 )
            {
                $Config['ContentType'] = 'jpeg';
                imagedestroy($GDtmp);
            }
            else
            {
                 $Config['ContentType'] = $OriginContentType[1];
                 imagedestroy($this->GD);
                 $this->GD = $GDtmp;
            }
        }

        // only pay attention to what we know - CropWH / MaxW / MaxH - ignores others
        // each takes a change string in query string format
        // these act on the original image aside from content type optimization above
        foreach( $Config as $K => $V )
        {
            if( $K === 'CropWH' )
            {
                $WH = explode('x',$V);

                if( count($WH) !== 2 || !ctype_digit($WH[0]) || !ctype_digit($WH[1]) )
                    throw new Exception('Invalid CropWH parameters.');

                $this->CropWH((int)$WH[0],(int)$WH[1]);
            }
            else if( $K === 'MaxW' || $K === 'MaxH' )
            {
                if( strpos($V,'&scale') !== FALSE )
                    $this->{$K}((int)str_replace('&scale','',$V),TRUE);
                else
                    $this->{$K}((int)$V,FALSE);
            }
        }

        // preserve transparency for certain types - the target GD resource needs it, not the source.
        if( $Config['ContentType'] === 'gif' || $Config['ContentType'] === 'png' )
        {
            imagealphablending($this->GD,FALSE);
            imagesavealpha($this->GD,TRUE);
        }

        // determine output and do it - we'll return a buffer using output buffering
        // some of this GD could potentially be optimized?
        ob_start();
        if( $Config['ContentType'] === 'gif' )
        {
            imagegif($this->GD);
        }
        else if( $Config['ContentType'] === 'jpeg' )
        {
            imageinterlace($this->GD,1);
            imagejpeg($this->GD);
        }
        else if( $Config['ContentType'] === 'png' )
        {
            imagepng($this->GD,NULL,9,PNG_ALL_FILTERS);
        }
        else
            throw new Exception("Unknown output ContentType '{$Config['ContentType']}'");

        return ob_get_contents();
    }


    protected function CropWH( $W,$H )
    {
        // see note on imagecopyresampled on php.net - maybe always use true color
        if( $this->GDColorCount === 0 )
            $GD = imagecreatetruecolor($W,$H);
        else
            $GD = imagecreate($W,$H);

        $SX = (int) ($W<$this->GDW?(($this->GDW-$W)/2):0);
        $SY = (int) ($H<$this->GDH?(($this->GDH-$H)/2):0);

        imagecopyresampled($GD,$this->GD,0,0,$SX,$SY,$W,$H,$W<$this->GDW?$W:$this->GDW,$H<$this->GDH?$H:$this->GDH);
        imagedestroy($this->GD);
        $this->GD = $GD;
        $this->GDW = $W;
        $this->GDH = $H;
    }

    public function MaxW( $W,$Scale )
    {
        if( $Scale )
        {
            $H = (int) ($this->GDH*($W/$this->GDW));
            $W2 = $this->GDW;
            $SX = 0;
        }
        else
        {
            $H = $this->GDH;
            $W2 = $W;
            $SX = (int) ($W<$this->GDW?(($this->GDW-$W)/2):0);
        }

        if( $this->GDColorCount === 0 )
            $GD = imagecreatetruecolor($W,$H);
        else
            $GD = imagecreate($W,$H);

        imagecopyresampled($GD,$this->GD,0,0,$SX,0,$W,$H,$W2,$this->GDH);
        imagedestroy($this->GD);
        $this->GD = $GD;
        $this->GDW = $W;
        $this->GDH = $H;
    }

    public function MaxH( $H,$Scale )
    {
        if( $Scale )
        {
            $W = (int) ($this->GDW*($H/$this->GDH));
            $H2 = $this->GDH;
            $SY = 0;
        }
        else
        {
            $W = $this->GDW;
            $H2 = $H;
            $SY = (int) ($H<$this->GDH?(($this->GDH-$H)/2):0);
        }

        if( $this->GDColorCount === 0 )
            $GD = imagecreatetruecolor($W,$H);
        else
            $GD = imagecreate($W,$H);

        imagecopyresampled($GD,$this->GD,0,0,0,$SY,$W,$H,$this->GDW,$H2);
        imagedestroy($this->GD);
        $this->GD = $GD;
        $this->GDW = $W;
        $this->GDH = $H;
    }

    public function CropWH2( $P )
    {
        $P = explode('x',$P);
        if( count($P) !== 2 || !ctype_digit($P[0]) || !ctype_digit($P[1]) )
            throw Exception('Invalid parameters.');

        $ORatio = $this->GD[1]/$this->GD[2];

        if( ($P[0]/$P[1]) > $ORatio )
        {
            $newH = $P[0]/$ORatio;
            $newW = $P[0];
        }
        else
        {
            $newW = $P[1]*$ORatio;
            $newH = $P[1];
        }

        $newWMid = $newW/2;
        $newHMid = $newH/2;

        $tmpGD = imagecreatetruecolor(round($newW),round($newH));
        imagecopyresampled($tmpGD,$this->GD[0],0,0,0,0,$newW,$newH,$this->GD[1],$this->GD[2]);

        $this->GD[0] = $tmpGD;return;

        $this->GD[0] = imagecreatetruecolor($P[0],$P[1]);
        imagecopyresampled($this->GD[0],$tmpGD,0,0,($newWMid-($P[0]/2)),($newHMid-($P[1]/2)),$P[0],$P[1],$P[0],$P[1]);
    }

    /**
     * Type is:
     * 	w = width
     *  h = height
     *  d = deci
     *
     *  This won't scale up such as $W/$Factor - which would basically make it a no-op in such a
     *  case and we should probably optimize that
     */
    protected static function PropScale( $W,$H,$Type,$Factor )
    {
        if( $Type === 'w' )
            $Scale = $Factor<$W?$Factor/$W:1;
        else if( $Type === 'h' )
            $Scale = $Factor<$H?$Factor/$H:1;
        else if( $Type === 'd' )
            $Scale = $Factor;

        return array($Scale*$W,$Scale*$H);
    }
}


// suited for prepending path segments to a filename
class Linkcnvyr extends \fw\LinkSet
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

        return \fw\URL::ToString($Base);
    }
}

