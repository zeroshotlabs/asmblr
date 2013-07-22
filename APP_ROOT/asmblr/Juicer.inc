<?php
namespace asm;

class Juicer
// extends DataSet
{
    protected $aapi;

    protected $RootURL = array();
    protected $RootURLStr = '';

    protected $RunID = '';

    protected $LOG;

    protected $TidyConfig = array('wrap'=>0,'output-html'=>TRUE,'indent'=>FALSE,'indent-spaces'=>4,'vertical-space'=>TRUE,
                                  'char-encoding'=>'utf8','output-bom'=>FALSE);

    public $RootScheme = '';
    public $RootDomain = '';
    public $RootPath = '';

    public $Content = array();
    public $Pages = array();

    // Root() must be called before anything else
    public function __construct( \asm\AAPIClient $aapi )
    {
        $this->aapi = $aapi;
    }

    // sets root URL used to determine whether other URLs are internal or external
    // sets Tag for storage of each page
    public function Root( $URL,$Site )
    {
        // dangerous!
        set_time_limit(0);

        $this->RootURL = \fw\URL::Init($URL);
        $this->RootURLStr = \fw\URL::ToString($this->RootURL);

        if( empty($this->RootURL['Hostname']) )
            throw new Exception('Root() URL must contain a domain.');

        $this->RootScheme = $this->RootURL['Scheme'];
        $this->RootDomain = \fw\URL::Hostname($this->RootURL);
        $this->RootPath = \fw\URL::Path($this->RootURL);

        $this->RunID = $this->RootDomain.'_'.\time();

        $this->Tag = $this->RootDomain.$this->RootPath;

        $this->Content = $this->Pages = array();

        $CrawlDir = "/tmp/{$this->RunID}";
        $this->LOG = fopen("/tmp/{$this->RunID}.log",'w');

        \fw\Log::Sys("LOGGING TO /tmp/{$this->RunID}.log");

        $this->aapi->POST('site_set_status',array('Status'=>'Juicing','Site_id'=>$Site['_id']));

        \fw\Log::Sys('Running wget...');

        fwrite($this->LOG,"\n\nCRAWL: wget -S -nv -r -l 1 -p -nH -E -P $CrawlDir {$this->RootURLStr} 2>&1");

        $O = array();
        exec("wget -S -nv -r -l 1 -p -nH -E -P $CrawlDir {$this->RootURLStr} 2>&1",$O);

        fwrite($this->LOG,"\n\n".implode("\n",$O));

//$_SESSION['O'] = $O;
//$O = $_SESSION['O'];
//var_dump($O);

        $LastBuf = array();
        foreach( $O as $K => $V )
        {
            if( stripos($V,'FINISHED ') === 0 )
            {
                break;
            }
            else if( stripos($V,'Content-Type:') !== FALSE )
            {
                $LastBuf['ContentType'] = trim(substr($V,15));
            }
            else if( $V[0] !== ' ' && strpos($V,'URL:') !== FALSE )
            {
                $M = array();
                $R = preg_match('/URL:(.*?) /',$V,$M);
                if( $R === 1 )
                    $LastBuf['URL'] = trim($M[1]);

                $M = array();
                $R = preg_match('/\"(.*?)\"/',$V,$M);
                if( $R === 1 )
                    $LastBuf['LocalPath'] = trim($M[1]);

//                 // usually something like a 404
//                 if( empty($LastBuf['ContentType']) )
//                 {
//                     \fw\Log::Sys($V);
//                     continue;
//                 }

                // a page
                if( stripos($LastBuf['ContentType'],'text/html') !== FALSE )
                {
                    list($Linker,$Handle,$URL) = $this->URLr($LastBuf['URL'],'lp');
                    $LastBuf['Linker'] = $Linker;
                    $LastBuf['Handle'] = $Handle;
                    $LastBuf['URL'] = $URL;

                    // using handle probably overwrites cases like /about/contactus and /contactus
                    // handle possibly should contain all segments separated by underscore
                    $this->Pages[$Handle] = $LastBuf;

                    fwrite($this->LOG,"\n\nPAGE: {$Linker} ".\fw\URL::ToString($URL));
                }
                // content
                else
                {
                    list($Linker,$Handle,$URL) = $this->URLr($LastBuf['URL'],'lc');
                    $LastBuf['Linker'] = $Linker;
                    $LastBuf['Handle'] = $Handle;
                    $LastBuf['URL'] = $URL;

                    $this->Content[$Handle] = $LastBuf;

                    fwrite($this->LOG,"\n\nCONTENT: {$Linker} ".\fw\URL::ToString($URL));
                }

                $LastBuf = array();
            }
            // anything else is probably a failed request like 404
        }

        // normalize all crawled pages
        // establish the Base page as the first HTML content type encountered (could also check Path I guess)
        // if a page is first-level (1 path segment and not root) diff it
        $Base = $BaseHTML = $BaseHTMLLines = NULL;
        // we want to figure out the max length for each block which will become templates
        $TemplateBlocks = array();
        // these are the line changes - always single line on Base but could map to multiple on children
        $PageLines = array();
        foreach( $this->Pages as &$Page )
        {
            $this->NormalizePage($Page);

            if( empty($Base) && stripos($Page['ContentType'],'text/html') !== FALSE )
            {
                $Base = $Page;
                $BaseHTML = file_get_contents($Page['LocalPath']);
                $BaseHTMLLines = explode("\n",$BaseHTML);
            }
            else if( count($Page['URL']['Path']['Segments']) === 1 )
            {
                fwrite($this->LOG,"\n\nBASE DIFF PAGE {$Page['LocalPath']}");
                $Page['Blocks'] = $this->diff($BaseHTML,file_get_contents($Page['LocalPath']));
                fwrite($this->LOG,"\n\n".\fw\Debug::Dump($Page['Blocks']));

                // determine maxes of blocks and aggregate line changes
                foreach( $Page['Blocks'] as $K => $B )
                {
                    if( strpos($K,'Block') === 0 )
                    {
                        if( empty($TemplateBlocks[$K]) )
                            $TemplateBlocks[$K] = $B;
                        else if( $B[1] > $TemplateBlocks[$K][1] )
                            $TemplateBlocks[$K] = $B;
                    }
                    else
                    {
                        // can this overwrite?
                        $PageLines[$K] = $B;
                    }
                }
            }
        }

        // the gold has been squeezed!  these are used to cut templates out of Base and subsequently
        // compare (somehow) against all of the pages sites
        // first inject single lines with $page since they won't change line numbering - we ignore lengths other than 1 for now
        foreach( $PageLines as $K => $PL )
        {
            // implode("\n",array_slice($CS['Lines'],$C[2],$C[3]));
            if( $PL[1] === 1 )
            {
                $Base['PageLines'][$K] = $BaseHTMLLines[$PL[0]+1];
                $BaseHTMLLines[$PL[0]+1] = "<?=eval('?>'.\$page->Base{$K})?>";
            }
        }

        // now do blocks
        foreach( $TemplateBlocks as $K => $TB )
        {
            $T = array_splice($BaseHTMLLines,$TB[0]-1,$TB[1]+1,"<?php \$html->Base{$K}(); ?>");
            $Base['TemplateBlocks'][$K] = implode("\n",$T);
        }

        $BaseHTML = implode("\n",$BaseHTMLLines);

        // go through all pages, diffing and storing
        // probably naming confusion between juicer pages and asmblr pages
        foreach( $this->Pages as $Page )
        {
            $PathStr = \fw\Path::ToURLString($Page['URL']['Path']);

            $P = $this->CreatePage($PathStr,$Page['Handle'],$Page['Title'],$Page['Description'],$Page['Keywords'],$Site['_id']);
            if( empty($P) )
                continue;

            $PageHTML = file_get_contents($Page['LocalPath']);
            $PageHTMLLines = explode("\n",$PageHTML);

            // need detection of tiny or crazy different files, like robots.txt

            $Blocks = $this->diff($BaseHTML,$PageHTML);

            // if a block/line isn't in Base, we skip it for now
            foreach( $Blocks as $K => $B )
            {
                if( isset($Base['PageLines'][$K]) )
                {
                    $L = implode("\n",array_slice($PageHTMLLines,$B[0]+1,$B[3]));
                    $this->aapi->POST('page_set_directive',array('Name'=>'page','Key'=>"Base{$K}",'Value'=>$L,'Page_id'=>$P['_id']));
                }
                else if( isset($Base['TemplateBlocks'][$K]) )
                {
                    $L = implode("\n",array_slice($PageHTMLLines,$B[2]-1,$B[3]+1));
                    $this->aapi->POST('template_create',array('Name'=>$Page['Handle'].$K,'Site_id'=>$Site['_id'],'Body'=>$L));
                    $this->aapi->POST('page_set_directive',array('Name'=>'html','Key'=>"Base{$K}",'Value'=>$Page['Handle'].$K,'Page_id'=>$P['_id']));
                }
                else
                    fwrite($this->LOG,"\n\nSKIPPED BLOCK $K");
            }

            \fw\Log::Sys("Page OK: {$Page['LocalPath']}");

/*
            // no blocks so just store with Base template
            if( $P['Blocks'] === FALSE )
            {
                $P = $this->CreatePage($PathStr,$Page['Handle'],$Page['Title'],$Page['Description'],$Page['Keywords'],$Site['_id']);
                if( empty($P) )
                    continue;

                \fw\Log::Sys("Page OK: {$PathStr}");

                $this->aapi->POST('template_create',array('Name'=>$Page['Handle'],'Site_id'=>$Site['_id'],'Body'=>$PageHTML));
                $this->aapi->POST('page_set_directive',array('Name'=>'html','Key'=>'Base','Value'=>$Page['Handle'],'Page_id'=>$P['_id']));
            }
*/

        }



        // and pull in all content that's been aggregated - maybe more aggregation ... from CSS?
        foreach( $this->Content as &$Content )
        {
            if( stripos($Content['ContentType'],'text/css') !== FALSE )
                $this->NormalizeCSS($Content);

            $PathStr = \fw\Path::ToURLString($Content['URL']['Path']);

            $C = $this->CreateContent($PathStr,'',$Site['_id'],$Content['LocalPath']);
            if( empty($C) )
                continue;

            \fw\Log::Sys("Content OK: {$Content['LocalPath']}");
        }


        // do site directives based on Base and create the Base template
        $this->aapi->POST('site_set_directive',array('Name'=>'page','Key'=>'Title','Value'=>$Base['Title'],'Site_id'=>$Site['_id']));
        $this->aapi->POST('site_set_directive',array('Name'=>'page','Key'=>'Description','Value'=>$Base['Description'],'Site_id'=>$Site['_id']));
        $this->aapi->POST('site_set_directive',array('Name'=>'page','Key'=>'Keywords','Value'=>$Base['Keywords'],'Site_id'=>$Site['_id']));

        $this->aapi->POST('template_create',array('Name'=>'Base','Site_id'=>$Site['_id'],'Body'=>$BaseHTML));

        $this->CreateContentSrv($Site['_id']);





        var_dump(count($this->Pages));

        unlink($CrawlDir);

exit;

        var_dump($Base['PageLines']);
        var_dump($Base['TemplateBlocks']);

        var_dump($TemplateBlocks);
        var_dump($PageLines);


exit;


    }

    // normalize URL (external/internal based on RootURL) and return proper linker code
    // and replace with proper $Linker ($lp, $lc) - $Linker is just a string
    protected function URLr( $OURL,$Linker )
    {
        $OURL = trim($OURL);

        // leave empty or fragments alone
        if( empty($OURL) || $OURL[0] === '#' || strpos($OURL,'javascript:') !== FALSE )
            return array('','','');

        // leave protocol-relative links alone
        if( strpos($OURL,'//') === 0 )
            return array('',$OURL,$OURL);

        // do we need to do anything special with just / ?

        $URL = \fw\URL::Init($OURL);

        // hostname is empty or matches root, so must be on-site link
        // could probably do smarter hostname matching
        if( empty($URL['Hostname']) || (\fw\URL::Hostname($URL) === $this->RootDomain) )
        {
            $URL['Scheme'] = $this->RootScheme;
            $URL['Hostname'] = $this->RootURL['Hostname'];
        }
        // off site link so don't touch and there's no linker
        else
        {
            $T = \fw\URL::ToString($URL);
            return array('',$T,$T);
        }

        // treat as page - drop the extension of filename and use as page name
        if( $Linker === 'lp' )
        {
            $Handle = $this->Path2Handle($URL['Path'],'lp');
            return array('<?=$lp(\''.$Handle.'\')?>',$Handle,$URL);
        }
        // treat as content - make path not abs/dir and use as content path/handle
        else if( $Linker === 'lc' )
        {
            $Handle = $this->Path2Handle($URL['Path'],'lc');
            return array('<?=$lc(\''.$Handle.'\')?>',$Handle,$URL);
        }
        else
            throw new Exception("Don't know what $Linker is.");
    }

    protected function Path2Handle( $Path,$Type )
    {
        if( $Type === 'lp' )
        {
            // hardwired hack which confuses external links to other home pages
            if( $Path['IsRoot'] === TRUE )
                return 'ROOT';

            // pages have an absolute path always
            $Path['IsAbs'] = TRUE;

            $Segments = array();
            foreach( $Path['Segments'] as $S )
                $Segments[] = pathinfo($S,PATHINFO_FILENAME);

            return strtolower(implode('_',$Segments));
        }
        else if( $Type === 'lc' )
        {
            // content are always relative and never a dir
            $Path['IsDir'] = $Path['IsAbs'] = FALSE;
            return \fw\Path::ToURLString($Path);
        }
        else
            throw new Exception("Unknown Path2Handle() Type '$Type'");
    }

    /**
     * juicing processes a URL, prepping for it to become a Page, Templates, and Content
     *  - tidy of html and split to head and body
     *  - for the head, extract URLs, title and meta content into $lc() and $page
     *      - store list of CSS and JS URLs as Structs
     *      - URLs are normalized as internal or external
     *  - for the body, extract URLs into $lp
     *      - store list of URLs as Structs
     *      - URLs are normalized as internal or external
     *
     * normalizes a crawled HTML page (not a Page struct nessecarily)
     *
     * @param unknown $URL
     */
    protected function NormalizePage( &$Page )
    {
        // we ignore others, including content type, since asmblr will handle that
        // the HTML is written back to disk, not stored in memory
        $Page['Title'] = '';
        $Page['Description'] = '';
        $Page['Keywords'] = '';
        // will hold our one liner changes
        $Page['PageLines'] = array();
        // will hold our template blocks
        $Page['TemplateBlocks'] = array();

        $PageLinks = $ContentLinks = array();

        $HTML = trim(file_get_contents($Page['LocalPath']));
        \fw\Struct::ToUTF8($HTML);

        // if there are no tags, don't do anything except write back the content
        if( stripos($HTML,'<html') === FALSE && stripos($HTML,'<body') === FALSE )
        {
            file_put_contents($Page['LocalPath'],$HTML);
            return;
        }

        $HTML = $this->normalize_buf($HTML);

        libxml_use_internal_errors(TRUE);
        $DOM = new \DOMDocument;
        $DOM->strictErrorChecking = FALSE;
        $DOM->preserveWhiteSpace = TRUE;
        $DOM->formatOutput = TRUE;
        $DOM->xmlStandalone = TRUE;
        $DOM->recover = TRUE;
        $DOM->resolveExternals = FALSE;
        @$DOM->loadHTML($HTML);

        $XPR = new \DOMXPath($DOM);

        $R = $XPR->query('//title');
        foreach( $R as $V )
        {
            if( !empty($V->textContent) )
                $Page['Title'] = htmlentities($V->textContent);

            $Title = preg_quote("<title>{$Page['Title']}</title>",'/');
            $HTML = preg_replace("/$Title/",'<title><?=$this($page->Title)?></title>',$HTML);
        }

        $R = $XPR->query('//meta[@name="description"]/@content');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Page['Description'] = htmlentities($V->value);

            $Description = preg_quote("content=\"{$Page['Description']}\"",'/');
            $HTML = preg_replace("/{$Description}/",'content="<?=$this($page->Description)?>"',$HTML);
        }

        $R = $XPR->query('//meta[@name="keywords"]/@content');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Page['Keywords'] = htmlentities($V->value);

            $Keywords = preg_quote("content=\"{$Page['Keywords']}\"",'/');
            $HTML = preg_replace("/{$Keywords}/",'content="<?=$this($page->Keywords)?>"',$HTML);
        }

        $R = $XPR->query('//link[@rel="stylesheet"]/@href');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $ContentLinks[] = $V->value;
        }

        $R = $XPR->query('//script/@src');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $ContentLinks[] = $V->value;
        }

        $R = $XPR->query('//a/@href');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $PageLinks[] = $V->value;
        }

        $R = $XPR->query('//img/@src');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $ContentLinks[] = $V->value;
        }

        // and finally replace the links on the page with our linker code, which will become themselves pages
        // handles both single/double quotes as ='' or =""
        foreach( $PageLinks as $Raw )
        {
            list($Linker,$Handle,$URL) = $this->URLr($Raw,'lp');
            // skip external links or unknown (previously uncrawled) pages
            if( empty($Linker) || empty($this->Pages[$Handle]['Linker']) )
                continue;

            $Link = preg_quote("{$Raw}",'/');
            $HTML = preg_replace(array("/=\"{$Link}\"/","/='{$Link}'/"),array("=\"{$this->Pages[$Handle]['Linker']}\"","='{$this->Pages[$Handle]['Linker']}'"),$HTML);
        }

        // normalize links to content/pages and put in our linker, based on Handles of our crawled pages/content
        // ditto matching as above
        foreach( $ContentLinks as $Raw )
        {
            list($Linker,$Handle,$URL) = $this->URLr($Raw,'lc');
            // skip external links or unknown (previously uncrawled) content
            if( empty($Linker) || empty($this->Content[$Handle]['Linker']) )
                continue;

            $Link = preg_quote("{$Raw}",'/');
            $HTML = preg_replace(array("/=\"{$Link}\"/","/='{$Link}'/"),array("=\"{$this->Content[$Handle]['Linker']}\"","='{$this->Content[$Handle]['Linker']}'"),$HTML);
        }

        file_put_contents($Page['LocalPath'],$HTML);
    }

    protected function NormalizeCSS( &$Content )
    {
        $HTML = trim(file_get_contents($Content['LocalPath']));
        \fw\Struct::ToUTF8($HTML);

        $Matches = array();
        preg_match_all('/url\s*\(([^\)]+)\)/',$HTML,$Matches);

        if( empty($Matches[1]) )
        {
            file_put_contents($Content['LocalPath'],$HTML);
            return;
        }

        foreach( $Matches[1] as $V )
        {
            // csrv prefix is hardwired and there's no linker (since Content isn't currently rendered)
            // this probably needs to change somehow (maybe CSS are pages/templates)
            $URL['Path'] = \fw\Path::Init('/csrv/'.$V);
            $CSSPath = \fw\Path::ToURLString($URL['Path']);

            $Link = preg_quote("({$V})",'/');
            $HTML = preg_replace("/{$Link}/","({$CSSPath})",$HTML);
        }

        file_put_contents($Content['LocalPath'],$HTML);

/*
                $URL['Path'] = \fw\Path::Init($V);
                $URL['Path']['IsAbs'] = TRUE;
                $ImgBody = file_get_contents(\fw\URL::ToString($URL));

                $URL['Path']['IsAbs'] = $URL['Path']['IsDir'] = FALSE;
                $ImgPath = \fw\Path::ToURLString($URL['Path']);

                $C = $this->aapi->POST('content_create',array('Path'=>$ImgPath,'Body'=>$ImgBody,'Site_id'=>$Site_id));
*/
    }

    // yeah yeah - we're diffing actual files now, though being able to use strings is real handy
    // returns a Blocks array, determined from Chunks
    protected function diff( $Parent,$Child )
    {
        // actually not good since it urlencodes the data apparently
//        $PFD = fopen('data://text/plain,'.$Parent,'r');
//        $CFD = fopen('data://text/plain,'.$Child,'r');

        $Blocks = array();

        $Pct = \fw\Math::Percentage(strlen($Child),strlen($Parent));
        // big difference between page size - dont bother (will get set as Base itself)
        if( ((int) trim($Pct,'%')) < 3 )
            return array();

        $PFD = fopen('php://temp','rw');
        fwrite($PFD,$Parent);

        $CFD = fopen('php://temp','rw');
        fwrite($CFD,$Child);

        $FDS = array(
           1 => array("pipe", "w"),
           3 => $PFD,
           4 => $CFD
        );

//        $P = proc_open('wdiff -3  /dev/fd/3 /dev/fd/4',$FDS,$PIPES);

        $P = proc_open('diff -i -w -B --strip-trailing-cr -d -U 0 /dev/fd/3 /dev/fd/4',$FDS,$PIPES);

        $Diff = stream_get_contents($PIPES[1]);
        fwrite($this->LOG,"\n\n".$Diff);

        $Diff = explode("\n",$Diff);

        $Chunks = array();
        foreach( $Diff as $K => $L )
        {
            if( in_array(substr($L,0,3),array('---','+++'),TRUE) )
                continue;

            if( substr($L,0,2) === '@@' )
            {
                $Chunk = array();
                preg_match('/@@ \-(\d+),?(\d+)? \+(\d+),?(\d+){0,1} @@/',$L,$Chunk);
                if( empty($Chunk[0]) )
                {
                    echo "\nInvalid chunk '{$L}'\n";
                    continue;
                }
                else
                {
                    $Chunk = array((int)$Chunk[1],(int)($Chunk[2]===''?1:$Chunk[2]),(int)$Chunk[3],isset($Chunk[4])?(int)$Chunk[4]:1);
                }

                $Chunks[] = $Chunk;
            }
        }

        $ChunkCount = count($Chunks);

        // convert chunks to blocks as determine one liner changes and combine contiguous chunks
        $Blocks = array();
        for( $i = 0; $i < $ChunkCount; ++$i )
        {
            $Chunk = &$Chunks[$i];
            if( empty($Chunk) )
                continue;

            // a one line change, or zero lines on the base
            if( $Chunk[1] === 1 && $Chunk[3] === 1 || ($Chunk[1] === 0) )
            {
                // one line blocks are named as the line number they start on - can this overwrite an existing?
                $Blocks["Line{$Chunk[0]}"] = $Chunk;
            }
            // same line but different length of change - start looking to combine contiguous
            else if( $Chunk[0] === $Chunk[2] && $Chunk[1] !== $Chunk[3] )
            {
                for( $j = 1; ($i+$j) < $ChunkCount; )
                {
                    $Chunk2 = &$Chunks[$i+$j];
                    if( $Chunk2[0] === ($Chunk[0]+$Chunk[1]+1) )
                    {
                        $Chunk[1] += ($Chunk2[1]+1);
                        $Chunk[3] += ($Chunk2[3]+1);
                        $Chunk2 = array();
                        ++$j;
                    }
                    else
                        break;
                }

                // can this ever overwrite an existing?
                $Blocks["Block{$Chunk[0]}"] = $Chunk;
            }
            // so if there isn't a block with same starting lines between parent/child, this does nothing...?
        }

        return $Blocks;


        // child, diff (each as arrays)
//        return array(explode("\n",$Child),explode("\n",stream_get_contents($PIPES[1])));
    }

    protected function normalize_buf( $buf )
    {
/*
 * // data:// isn't really good
        $PFD = fopen('data://text/plain,'.$buf,'r');

        $FDS = array(
           0 => $PFD,
           1 => array("pipe","w")
        );

        $P = proc_open('java -jar /usr/local/bin/tika-app-1.4.jar --html',$FDS,$PIPES);

        $buf = stream_get_contents($PIPES[1]);
*/

        $TR = new \tidy;
        $TR->parseString($buf,$this->TidyConfig);
        $TR->cleanRepair();
        $buf = (string) $TR;

        $h = '';
        foreach( explode("\n",str_replace(array("\r","\t"),array('',' '),$buf)) as $V )
        {
            $V = trim($V);
            if( $V === '' )
                continue;
            else
                $h .= $V."\n";
        }

        return $h;
    }

    // this uses curlfile if LocalPath is specified in which case Body is ignored and multipart encode upload is done
    protected function CreateContent( $Path,$Body,$Site_id,$LocalPath = '' )
    {
        if( !empty($LocalPath) )
            $Body = curl_file_create($LocalPath);

        $C = $this->aapi->POST('content_create',array('Path'=>$Path,
                                                      'Body'=>$Body,'Site_id'=>$Site_id));


//         if( empty($C) )
//         {
//             var_dump($this->aapi->ConnectionError);

//             var_dump($this->aapi->GetInfo());

//             var_dump($this->aapi->Response);
//         }


        if( $C['Status'] !== TRUE )
        {
            \fw\Log::Sys("Content Error: {$C['Msg']} ({$Path})");
            return array();
        }

        return $C;
    }

    protected function CreatePage( $Path,$Name,$Title,$Description,$Keywords,$Site_id )
    {
        $P = $this->aapi->POST('page_create',array('Path'=>$Path,'Name'=>$Name,'Site_id'=>$Site_id));

        if( $P['Status'] !== TRUE )
        {
            \fw\Log::Sys("Page Error: {$P['Msg']} ({$Path})");
            return array();
        }

        $P = $P['Data'];

        $this->aapi->POST('page_set_directive',array('Name'=>'page','Key'=>'Title','Value'=>$Title,'Page_id'=>$P['_id']));
        $this->aapi->POST('page_set_directive',array('Name'=>'page','Key'=>'Description','Value'=>$Description,'Page_id'=>$P['_id']));
        $this->aapi->POST('page_set_directive',array('Name'=>'page','Key'=>'Keywords','Value'=>$Keywords,'Page_id'=>$P['_id']));

        $this->aapi->POST('page_set_status',array('Status'=>'Active','Page_id'=>$P['_id']));

        return $P;
    }

    protected function CreateContentSrv( $Site_id )
    {
        $Routine = <<<'_EO_ROUTINE_'
// for URL direct delivery - this hard exits, sets headers, etc.

ini_set('zlib.output_compression',FALSE);

$MP = asm()->MatchPath;
// need to be configurable somehow inline with csrv in asmboot
$MP['Segments'] = array_slice($MP['Segments'],1);
//$MP['IsDir'] = $MP['IsAbs'] = FALSE;
//var_dump(\fw\Path::ToURLString($MP));

$C = asm('content')->Match(asm()->SrvSite['_id'],$MP);
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
        header("Content-Type: {$C['Type']}");
        // var_dump($C['Length']);
        // var_dump(strlen($C['Body']->bin));
        // header("Content-Length: {$C['Length']}");
        // TODO: honor disposition, cache, etc. from Meta
        // TODO: optimize for x-sendfile, stream from mongo, etc.
        if( is_object($C['Body']) )
            echo $C['Body']->bin;
        else
            echo $C['Body'];
    }
}

exit;

_EO_ROUTINE_;

        $P = $this->aapi->POST('page_create',array('Path'=>'/csrv/','Name'=>'ContentSrv','Site_id'=>$Site_id));
        if( $P['Status'] === TRUE )
        {
            $this->aapi->POST('page_set_routine',array('Routine'=>$Routine,'Page_id'=>$P['Data']['_id']));
            $this->aapi->POST('page_set_status',array('Status'=>'Active','Page_id'=>$P['Data']['_id']));
        }
    }


}

/*
var_dump($Header);
exit;

//        $Description = $XPR->query('//a/@href');

        foreach( $Description as $D )
            var_dump($D->value);
        var_dump((string)$Description->item(0)->value);
        exit;

        $DOMHead = $DOM->getElementsByTagName('head')->item(0);
        $DOMBody = $DOM->getElementsByTagName('body')->item(0);

        if( empty($DOMHead) )
            $DOMHead = $DOM->insertBefore($DOM->createElement('head'),$DOM->firstChild);

        if( empty($DOMBody) )
            $DOMBody = $DOM->appendChild($DOM->createElement('body'));

        $T = $DOMHead->getElementsByTagName('title');
        if( $T->length > 0 )
        {
            $Header['Title'] = $T->item(0)->nodeValue;
//            str_replace("<title>{$Header['Title']}</title>",'<title><?=$page->Title\</title>',$Header['HTML']);
            $T->item(0)->nodeValue = '<?=$page->Title?>';
        }

        foreach( $DOMHead->getElementsByTagName('meta') as $K => $V )
        {
            $Name = strtolower($V->getAttribute('name'));

            if( $Name === 'description' || $Name === 'keywords' )
            {
                $Name = ucfirst($Name);
                $Header[$Name] = $V->getAttribute('content');
                $V->setAttribute('content',"<?=\$page->{$Name}?>");
            }
        }

        // this will probably choke on things like canonical, index, prev, next stuff, among others
        foreach( $DOMHead->getElementsByTagName('link') as $K => $V )
        {
            $URL = $V->getAttribute('href');

            if( strpos($URL,'.css') !== FALSE || $V->getAttribute('rel') === 'stylesheet' || $V->getAttribute('type') === 'text/css' )
            {
                list($Linker,$Handle,$URL) = $this->URLr($URL,'lc');
                if( empty($URL) )
                    continue;
                $V->setAttribute('href',$Linker);
                $Header['CSS'][$Handle] = $URL;
            }
        }

        foreach( $DOMHead->getElementsByTagName('script') as $K => $V )
        {
            $URL = $V->getAttribute('src');

            if( strpos($URL,'.js') !== FALSE || $V->getAttribute('language') === 'javascript' )
            {
                list($Linker,$Handle,$URL) = $this->URLr($URL,'lc');
                if( empty($URL) )
                    continue;
                $V->setAttribute('src',$Linker);
                $Header['JS'][$Handle] = $URL;
            }
        }
*/
/*
        $Header['HTML'] = $DOM->saveHTML($DOM->getElementsByTagName('head')->item(0));

        foreach( $DOMBody->getElementsByTagName('a') as $K => $V )
        {
            $URL = $V->getAttribute('href');
            list($Linker,$Handle,$URL) = $this->URLr($URL,'lp');
            if( empty($URL) )
                continue;

            $V->setAttribute('href',$Linker);
            $Body['Links'][$Handle] = $URL;
        }
*/
/*
        $Body['HTML'] = $DOM->saveHTML($DOM->getElementsByTagName('body')->item(0));

        // retidy everything, which may be wasteful
        unset($TR);

        $TR = new \tidy;
        $TR->parseString($Header['HTML'].$Body['HTML'],$this->TidyConfig);
        $TR->cleanRepair();
//        var_dump($Header['HTML']);
//        var_dump((string) $TR->head());
        $Header['HTML'] = html_entity_decode((string) $TR->head());
        $Body['HTML'] = html_entity_decode((string) $TR->body());
var_dump($Header['HTML']);exit;
        return array('Header'=>$Header,'Body'=>$Body);
    }
*/



/*
    // standardize the HTML by replace title/desc/keywords, assets, and links with our placeholders and linkers
    protected function Screen( &$Juiced )
    {
//        if( !empty($Juiced['Title']) )
//        {
//        }

//        if( !empty($Juiced['Description']) )
//        {
//        }

//        if( !empty($Juiced['Keywords']) )
//        {
//        }

        // and replace assets with our linker code
        // assets are de-duped based on the fully qualified URL they are determined to live at
        foreach( $Juiced['Assets'] as &$Raw )
        {
            list($Linker,$Handle,$URL) = $this->URLr($Raw,'lc');

            // external link or bogus URL
            if( empty($Linker) )
                continue;

            $Link = preg_quote("\"{$Raw}\"",'/');
            $Juiced['HTML'] = preg_replace("/{$Link}/","\"{$Linker}\"",$Juiced['HTML']);

            $URLStr = \fw\URL::ToString($URL);

//            if( !isset($this->Content[$URLStr]) )
//                $this->Content[$URLStr] = array('Raw'=>$Raw,'Linker'=>$Linker,'Handle'=>$Handle,'URL'=>$URL);

            $Raw = array('Raw'=>$Raw,'Linker'=>$Linker,'Handle'=>$Handle,'URL'=>$URL);
        }

        // and finally replace the links on the page with our linker code, which will become themselves pages
        foreach( $Juiced['Links'] as &$Raw )
        {
            list($Linker,$Handle,$URL) = $this->URLr($Raw,'lp');

            // external link or bogus URL
            if( empty($Linker) )
                continue;

            $Link = preg_quote("\"{$Raw}\"",'/');
            $Juiced['HTML'] = preg_replace("/{$Link}/","\"{$Linker}\"",$Juiced['HTML']);

            $URLStr = \fw\URL::ToString($URL);

            if( $URLStr === $this->RootURLStr )
                continue;

//            if( !isset($this->Pages[$URLStr]) )
//                $this->Pages[$URLStr] = array('Raw'=>$Raw,'Linker'=>$Linker,'Handle'=>$Handle,'URL'=>$URL);
            $Raw = array('Raw'=>$Raw,'Linker'=>$Linker,'Handle'=>$Handle,'URL'=>$URL);
        }
    }
*/
