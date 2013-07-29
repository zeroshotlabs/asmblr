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
    public function Root( $URL,$Site,$RunID = '',$Depth = 3,$BaseTemplateOnly = FALSE )
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

        $this->Content = $this->Pages = array();

//        $RunID = 'www.new-york-divorce-attorney.com_1374542839';

        $this->aapi->POST('site_set_status',array('Status'=>'Juicing','Site_id'=>$Site['_id']));

        if( empty($RunID) )
        {
            $this->RunID = $this->RootDomain.'_'.\time();

            $this->LOG = fopen("/tmp/{$this->RunID}.log",'w');
            \fw\Log::Sys("LOGGING TO /tmp/{$this->RunID}.log");

            $CrawlDir = "/tmp/{$this->RunID}";
            $WgetLog = "/tmp/{$this->RunID}_wget.log";

            \fw\Log::Sys("Running wget into {$CrawlDir}...");

            $WgetCMD = "wget -S -nv -r -l {$Depth} -p -nH -E -o $WgetLog -P $CrawlDir {$this->RootURLStr} 2>&1";
            fwrite($this->LOG,"\n\nCRAWLing: {$WgetCMD}");
            exec($WgetCMD);

            $O = file($WgetLog);
        }
        else
        {
            $this->RunID = $RunID;
            $WgetLog = "/tmp/{$this->RunID}_wget.log";

            $this->LOG = fopen("/tmp/{$this->RunID}.log",'w');
            \fw\Log::Sys("LOGGING TO /tmp/{$this->RunID}.log");

            if( !is_readable($WgetLog) )
                throw new Exception("Cannot find prior run log $WgetLog");

            \fw\Log::Sys("Picking up from run log $WgetLog");

            $O = file($WgetLog);
        }

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

                // a page
                if( stripos($LastBuf['ContentType'],'text/html') !== FALSE )
                {
                    // may want to handle special cases index.html/index.php etc - / is already set as ROOT in Path2Handler
                    $URL = Juicer::FoldURL($LastBuf['URL'],$this->RootURL);
                    $Page = Page::Init($Site['_id'],Page::Path2Name($URL['Path']),\fw\Path::ToURLString($URL['Path']));
                    $Page['LocalPath'] = $LastBuf['LocalPath'];

                    if( !empty($this->Pages[$Page['Name']]) )
                    {
                        \fw\Log::Sys("Page Name '{$Page['Name']}' already seen in - skipping {$LastBuf['LocalPath']}");
                    }
                    else
                    {
                        $this->Pages[$Page['Name']] = $Page;
                        fwrite($this->LOG,"\n\nADDED CRAWLED PAGE: {$Page['Name']} ".\fw\URL::ToString($URL));
                    }
                }
                // content
                else
                {
                    $URL = Juicer::FoldURL($LastBuf['URL'],$this->RootURL);
                    // we'll set the body later
                    $Content = Content::Init($Site['_id'],\fw\Path::ToURLString($URL['Path']),'');
                    $Content['LocalPath'] = $LastBuf['LocalPath'];

                    if( !empty($this->Content[$Content['Path']]) )
                    {
                        \fw\Log::Sys("Content Handle '{$Content['Path']}' already exists in crawl - skipping {$LastBuf['LocalPath']}");
                    }
                    else
                    {
                        $this->Content[$Content['Path']] = $Content;
                        fwrite($this->LOG,"\n\nCRAWLED CONTENT: {$Content['Path']} ".\fw\URL::ToString($URL));
                    }
                }

                $LastBuf = array();
            }
        }

        // normalize crawled pages and establish the Base template as the first entry crawled (right?)
// should be moved to a method just for establishing the base and diffing, if we want to do that
/*
        $Base = $BaseHTML = $BaseHTMLLines = NULL;
        $TemplateBlocks = array();
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

                $ChildHTML = file_get_contents($Page['LocalPath']);
                $Page['Blocks'] = $this->diff($BaseHTML,$ChildHTML);
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
*/

        // just save as full Base template for now
        foreach( $this->Pages as &$Page )
        {
            $this->NormalizePage($Page);

            $PathStr = \fw\Path::ToURLString($Page['URL']['Path']);

            $PageHTML = file_get_contents($Page['LocalPath']);

            $P = $this->CreatePage($PathStr,$Page['Handle'],$Page['Title'],$Page['Description'],$Page['Keywords'],$Site['_id']);
            if( empty($P) )
                continue;

            $PageHTML = file_get_contents($Page['LocalPath']);
            $this->aapi->POST('template_create',array('Name'=>$Page['Handle'],'Site_id'=>$Site['_id'],'Body'=>$PageHTML));
            $this->aapi->POST('page_set_directive',array('Name'=>'html','Key'=>'Base','Value'=>$Page['Handle'],'Page_id'=>$P['_id']));

            fwrite($this->LOG,"\n\nBASE ONLY SAVE FOR {$Page['LocalPath']}");

            \fw\Log::Sys("Page OK: {$Page['LocalPath']}");
        }

        goto docontent;




/** remaining stuff shoudl be also in a method for diff processing **/








        fwrite($this->LOG,"\n\nPAGE LINES:\n".\fw\Debug::Dump($PageLines));
        fwrite($this->LOG,"\n\nTEMPLATE BLOCKS:\n".\fw\Debug::Dump($TemplateBlocks));

        // if the source site is a mess, good blocks won't be generated and we'll want to just store everything as Base
        if( empty($TemplateBlocks) )
        {
            $BaseOnly = TRUE;
        }
        else
        {
            $TotalInsert = 0;
            foreach( $TemplateBlocks as $TB )
                $TotalInsert += $TB[1];

            if( $TotalInsert < 20 && count($PageLines) > 5 )
                $BaseOnly = TRUE;
            else
                $BaseOnly = FALSE;
        }

        $BaseOnly = TRUE;

        if( $BaseOnly === FALSE )
        {
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
        }

        // go through all pages, diffing if BaseOnly = FALSE and storing
        // note the difference between a page array from juicer and that of asmblr itself
        // WOW PHP BUG: if this isn't &$Page it gets confused with above use of $Page in foreach
        foreach( $this->Pages as &$Page )
        {
            $PathStr = \fw\Path::ToURLString($Page['URL']['Path']);

            $PageHTML = file_get_contents($Page['LocalPath']);
            var_dump($PageHTML);
            exit;


            $P = $this->CreatePage($PathStr,$Page['Handle'],$Page['Title'],$Page['Description'],$Page['Keywords'],$Site['_id']);
            if( empty($P) )
                continue;

            $PageHTML = file_get_contents($Page['LocalPath']);
            $PageHTMLLines = explode("\n",$PageHTML);

            // need detection of tiny or crazy different files, like robots.txt

            if( $BaseOnly === TRUE )
            {
                $this->aapi->POST('template_create',array('Name'=>$Page['Handle'],'Site_id'=>$Site['_id'],'Body'=>$PageHTML));
                $this->aapi->POST('page_set_directive',array('Name'=>'html','Key'=>'Base','Value'=>$Page['Handle'],'Page_id'=>$P['_id']));

                fwrite($this->LOG,"\n\nBASE ONLY SAVE FOR {$Page['LocalPath']}");
            }
            else
            {
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

docontent:

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

        $this->aapi->POST('site_set_status',array('Status'=>'Inactive','Site_id'=>$Site['_id']));



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
        // the HTML will be written back to disk, not stored in memory
        $HTML = file_get_contents($Page['LocalPath']);

        $Page['Title'] = '';
        $Page['Description'] = '';
        $Page['Keywords'] = '';

        // if the HTML from disk contains a <?= it's been processed so skip
        // though need a better check
        if( strpos($HTML,'<?=') !== FALSE )
        {
            \fw\Log::Sys("PHP code detected - won't double process local path '{$Page['LocalPath']}'  skipping...");
            return;
        }

        // if there are no tags, don't do anything except UTF8 convert and write back to disk
        if( stripos($HTML,'<html') === FALSE && stripos($HTML,'<body') === FALSE )
        {
            \fw\Struct::ToUTF8($HTML);
            file_put_contents($Page['LocalPath'],$HTML);
            return;
        }

//        exec("java -jar /usr/local/bin/tagsoup-1.2.1.jar --html {$Page['LocalPath']} 2> /dev/null",$CleanedHTML);
//        $TR = new \tidy;
//        $TR->parseString(implode("\n",$CleanedHTML),$this->TidyConfig);
//        $TR->cleanRepair();
//        $CleanedHTML = (string) $TR;
//        file_put_contents($Page['LocalPath'],implode("\n",$CleanedHTML));
//        $CleanedHTML = array();
//        exec("java -jar /usr/local/bin/jtidy-r938.jar -i -wrap 2000 {$Page['LocalPath']} 2> /dev/null",$CleanedHTML);

/*
        $h = '';
        foreach( explode("\n",str_replace(array("\r","\t"),array('',' '),$buf)) as $V )
        {
            $V = trim($V);
            if( $V === '' )
                continue;
            else
                $h .= $V."\n";
        }
*/

        file_put_contents($Page['LocalPath'],$HTML);
    }


    /**
     * Pulls out meta (title, description, keywords) and links (a href, link CSS, script JS and img src)
     * from a chunk of HTML.
     *
     * For meta, each is replaced with a $page call.
     * For links, each is replaced with linker (lp/lc and handle).
     *
     * If AllowedPages/AllowedContent is not empty, it's taken as an associative array keyed by handle
     * which replaced links must exist in - otherwise they're skipped.
     *
     * If $KillSegments isn't empty, each segment will be removed from all paths before they are processing - no slashes.
     *
     * An array containing all raw stuff that was replaced is returned.
     */
    public static function ScreenHTML( $HTML,$RootURL,$AllowedPages = array(),$AllowedContent = array(),$KillSegments = array() )
    {
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

        $Screened = array('Title'=>'','Description'=>'','Keywords'=>'',
                          'Pages'=>array(),'CSS'=>array(),'JS'=>array(),'IMG'=>array());

        $R = $XPR->query('//title');
        foreach( $R as $V )
        {
            if( !empty($V->textContent) )
                $Screened['Title'] = htmlentities($V->textContent);
        }

        $R = $XPR->query('//meta[@name="description"]/@content');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Screened['Description'] = htmlentities($V->value);
        }

        $R = $XPR->query('//meta[@name="keywords"]/@content');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Screened['Keywords'] = htmlentities($V->value);
        }

        $R = $XPR->query('//link[@rel="stylesheet"]/@href');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Screened['CSS'][] = $V->value;
        }

        $R = $XPR->query('//script/@src');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Screened['JS'][] = $V->value;
        }

        $R = $XPR->query('//img/@src');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Screened['IMG'][] = $V->value;
        }

        $R = $XPR->query('//a/@href');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Screened['Pages'][] = $V->value;
        }

        if( !empty($Screened['Title']) )
        {
            $T = preg_quote("<title>{$Screened['Title']}</title>",'/');
            $HTML = preg_replace("/{$T}/",'<title><?=$this($page->Title)?></title>',$HTML);
        }

        if( !empty($Screened['Description']) )
        {
            $T = preg_quote("content=\"{$Screened['Description']}\"",'/');
            $HTML = preg_replace("/{$T}/",'content="<?=$this($page->Description)?>"',$HTML);
        }

        if( !empty($Screened['Keywords']) )
        {
            $T = preg_quote("content=\"{$Screened['Keywords']}\"",'/');
            $HTML = preg_replace("/{$T}/",'content="<?=$this($page->Keywords)?>"',$HTML);
        }

        // replace page links with $lp() linkers
        // handles both single/double quotes as ='' or =""
        // skips those not already known if AllowedPages isn't empty
        foreach( $Screened['Pages'] as &$Raw )
        {
            $T = static::ProcessRaw($Raw,'P',$RootURL,$KillSegments,$AllowedContent,$HTML);
            if( !empty($T) )
                $Raw = $T;
        }
/*

            try
            {
                $L = Juicer::FoldURL($Raw,$RootURL);
            }
            catch( \Exception $E )
            {
                echo "\nSkipped invalid raw page URL: {$Raw}";
                continue;
            }

            // bogus or external URL
            if( empty($L) )
                continue;

            if( !empty($KillSegments) )
            {
                foreach( $L['Path']['Segments'] as $K => $V )
                {
                    if( in_array($V,$KillSegments,TRUE) )
                        \fw\Path::Del($K,$L['Path']);
                }
            }

            $Name = Page::Path2Name($L['Path']);

            // the name is not in our list of allowed pages so skip
            if( !empty($AllowedPages) && empty($AllowedPages[$Name]) )
                continue;

            $Link = preg_quote("{$Raw}",'/');
            $Linker = "<?=\$lp('{$Name}')?>";
            $HTML = preg_replace(array("/=\"{$Link}\"/","/='{$Link}'/"),array("=\"{$Linker}\"","='{$Linker}'"),$HTML);

            $Screened['Pages'][$K] = array('Raw'=>$Raw,'Cleaned'=>$L);
 */
        // normalize links to content/pages and put in our linker, based on names of our crawled pages/content
        // ditto matching as above
        foreach( $Screened['CSS'] as &$Raw )
        {
            $T = static::ProcessRaw($Raw,'C',$RootURL,$KillSegments,$AllowedContent,$HTML);
            if( !empty($T) )
                $Raw = $T;
        }

        foreach( $Screened['JS'] as &$Raw )
        {
            $T = static::ProcessRaw($Raw,'C',$RootURL,$KillSegments,$AllowedContent,$HTML);
            if( !empty($T) )
                $Raw = $T;
        }

        foreach( $Screened['IMG'] as &$Raw )
        {
            $T = static::ProcessRaw($Raw,'C',$RootURL,$KillSegments,$AllowedContent,$HTML);
            if( !empty($T) )
                $Raw = $T;
        }

        $HTML2 = str_replace(array("\r","\t"),array('',' '),$HTML);
        $HTML2 = str_replace(array('></','> </','><','> <'),array(">\n</",">\n</",">\n<",">\n<"),$HTML2);

        $HTML = '';
        foreach( explode("\n",$HTML2) as $C )
        {
            $C = trim($C);
            if( empty($C) ) continue;
            $HTML .= trim($C)."\n";
        }

        \fw\Struct::ToUTF8($HTML);

        return array('Screened'=>$Screened,'HTML'=>$HTML);
    }

    protected static function ProcessRaw( $Raw,$Type,$RootURL,$KillSegments,$Allowed,&$HTML )
    {
        try
        {
            $L = Juicer::FoldURL($Raw,$RootURL);
        }
        catch( \Exception $E )
        {
            echo "\nSkipped invalid raw URL: ".substr($Raw,0,100);
            return FALSE;
        }

        // bogus or external URL
        if( empty($L) )
            return FALSE;

        if( !empty($KillSegments) )
        {
            foreach( $L['Path']['Segments'] as $K => $V )
            {
                if( in_array($V,$KillSegments,TRUE) )
                    \fw\Path::Del($K,$L['Path']);
            }
        }

        if( $Type === 'P' )
            $Name = Page::Path2Name($L['Path']);
        else
            $Name = Content::Path2Path($L['Path']);

        // the name is not in our list of allowed pages so skip
        if( !empty($Allowed) && empty($Allowed[$Name]) )
            continue;

        $Link = preg_quote("{$Raw}",'/');
        if( $Type === 'P' )
            $Linker = "<?=\$lp('{$Name}')?>";
        else
            $Linker = "<?=\$lc('{$Name}')?>";
        $HTML = preg_replace(array("/=\"{$Link}\"/","/='{$Link}'/"),array("=\"{$Linker}\"","='{$Linker}'"),$HTML);

        return array('Raw'=>$Raw,'Cleaned'=>$L);
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


    /**
     * Normalize raw URLs from web pages by "folding" them into a Root URL.
     * Skips fragments/javascript/protocol-relative and external URLs.
     *
     * If internal, fill in scheme/hostname.
     *
     * @param string $RawURL Original URL
     * @param array $RootURL Default URL to fill in parts that the raw URL may be missing.
     *
     * @retval array Normalized/folded URL struct.
     * @retval NULL URL not folded (JS/external/fragment/etc)
     */
    public static function FoldURL( $RawURL,$RootURL )
    {
        $URL = trim($RawURL);

        // leave empty or fragments or JS alone
        if( empty($URL) || $URL[0] === '#' || strpos($URL,'javascript:') !== FALSE )
            return NULL;

        // leave protocol-relative and data links alone
        if( strpos($URL,'//') === 0 || strpos($URL,'://data:') !== FALSE )
            return NULL;

        // do we need to do anything special with just / ?

        $URL = \fw\URL::Init($URL);

        // hostname is empty or matches root, so must be on-site link
        // could probably do smarter hostname matching including possibly using tags from the HTML itself
        if( empty($URL['Hostname']) || (\fw\URL::Hostname($URL) === \fw\URL::Hostname($RootURL)) )
        {
            $URL['Scheme'] = $RootURL['Scheme'];
            $URL['Hostname'] = $RootURL['Hostname'];
            return $URL;
        }
        // off site link so don't touch and there's no linker
        else
        {
            return NULL;
        }
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

//        $P = proc_open('wdiff   /dev/fd/3 /dev/fd/4',$FDS,$PIPES);
//        $P = proc_open('diff -i -w -B --strip-trailing-cr -d -U 0 /dev/fd/3 /dev/fd/4',$FDS,$PIPES);

        $P = proc_open('diff -i  -U 0 /dev/fd/3 /dev/fd/4',$FDS,$PIPES);

        $Diff = stream_get_contents($PIPES[1]);

        var_dump($Diff);
        exit;
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
