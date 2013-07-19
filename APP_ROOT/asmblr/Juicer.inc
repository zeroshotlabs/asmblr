<?php
namespace asm;

class Juicer
// extends DataSet
{
    protected $aapi;

    protected $RootURL = array();
    protected $RootURLStr = '';

    // set to the root URL
    protected $Tag = '';

    protected $TidyConfig = array('wrap'=>0,'drop-empty-paras'=>FALSE,'merge-divs'=>FALSE,'enclose-block-text'=>TRUE,'enclose-text'=>TRUE,
                             'output-html'=>TRUE,'indent'=>FALSE,'indent-spaces'=>4,'vertical-space'=>TRUE,
                             'char-encoding'=>'utf8','markup'=>FALSE,'output-bom'=>FALSE);

    public $RootScheme = '';
    public $RootDomain = '';
    public $RootPath = '';

    public $Content = array();
    public $Pages = array();


    public function __construct( \asm\AAPIClient $aapi )
    {
        $this->aapi = $aapi;
    }

/*
    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id )
    {
        parent::__construct($MDB,$Site_id,$this->Tag.$Site_id,'DirectiveSet');

        // Root() must be called before anything else
        $this->Tag = '';
    }
*/

    protected function CreateContent( $URL,$Site_id )
    {
        $URL['Path']['IsAbs'] = TRUE;
        $Path = \fw\Path::ToURLString($URL['Path']);
        $Body = file_get_contents(\fw\URL::ToString($URL));

        // if .css check for embedded images and pull in - no Linker is set for these so they're left as is in the css
        // this is hacked in here
        if( strpos($Path,'.css') !== FALSE )
        {
            $Matches = array();
            preg_match_all('/url\s*\(([^\)]+)\)/',$Body,$Matches);

            if( !empty($Matches[1]) )
            {
                foreach( $Matches[1] as $V )
                {
                    \fw\Log::Sys($V);
                    // csrv prefix is hardwired and there's no linker (since Content isn't currently rendered)
                    $URL['Path'] = \fw\Path::Init('/csrv/'.$V);
                    $CSSPath = \fw\Path::ToURLString($URL['Path']);

                    $Link = preg_quote("({$V})",'/');
                    $Body = preg_replace("/{$Link}/","({$CSSPath})",$Body);

                    $URL['Path'] = \fw\Path::Init($V);
                    $URL['Path']['IsAbs'] = TRUE;
                    $ImgBody = file_get_contents(\fw\URL::ToString($URL));

                    $URL['Path']['IsAbs'] = $URL['Path']['IsDir'] = FALSE;
                    $ImgPath = \fw\Path::ToURLString($URL['Path']);

                    $C = $this->aapi->POST('content_create',array('Path'=>$ImgPath,'Body'=>$ImgBody,'Site_id'=>$Site_id));
                }
            }
        }

        $C = $this->aapi->POST('content_create',array('Path'=>$Path,
                                                      'Body'=>$Body,'Site_id'=>$Site_id));

    }

//     // assets are de-duped based on the fully qualified URL they are determined to live at
//     // Raw and Handle are only kept for preg_replace() purposes
//     protected function ProcessContent( $Content )
//     {
//         foreach( $Content as $Raw )
//         {
//             list($Linker,$Handle,$URL) = $this->URLr($Raw,'lc');

//             if( empty($Linker) )
//                 continue;

//             $URLStr = \fw\URL::ToString($URL);

//             if( isset($this->Content[$URLStr]) )
//                 continue;

//             $this->Content[$URLStr] = array('Raw'=>$Raw,'Linker'=>$Linker,'Handle'=>$Handle,'URL'=>$URL);
//         }
//     }

    protected function CreatePage( $Path,$Name,$Title,$Description,$Keywords,$Template,$Site_id )
    {
        $P = $this->aapi->POST('page_create',array('Path'=>$Path,'Name'=>$Name,'Site_id'=>$Site_id));

        if( $P['Status'] !== TRUE )
        {
            return $P['Msg'];
        }

        $P = $P['Data'];

        $this->aapi->POST('template_create',array('Name'=>$Name,'Site_id'=>$Site_id,'Body'=>$Template));
        $this->aapi->POST('page_set_directive',array('Name'=>'html','Key'=>'Base','Value'=>$Name,'Page_id'=>$P['_id']));

        $this->aapi->POST('page_set_directive',array('Name'=>'page','Key'=>'Title','Value'=>$Title,'Page_id'=>$P['_id']));
        $this->aapi->POST('page_set_directive',array('Name'=>'page','Key'=>'Description','Value'=>$Description,'Page_id'=>$P['_id']));
        $this->aapi->POST('page_set_directive',array('Name'=>'page','Key'=>'Keywords','Value'=>$Keywords,'Page_id'=>$P['_id']));

        $this->aapi->POST('page_set_status',array('Status'=>'Active','Page_id'=>$P['_id']));

        return $P['Path'];
    }

//     protected function ProcessPages( $Pages )
//     {
//         foreach( $Pages as $Raw )
//         {
//             list($Linker,$Handle,$URL) = $this->URLr($Raw,'lp');

//             if( empty($Linker) )
//                 continue;

//             $URLStr = \fw\URL::ToString($URL);

//             if( $URLStr === $this->RootURLStr )
//                 continue;

//             if( isset($this->Pages[$URLStr]) )
//                 continue;

//             $this->Pages[$URLStr] = array('Raw'=>$Raw,'Linker'=>$Linker,'Handle'=>$Handle,'URL'=>$URL);
//         }
//     }

    // sets root URL used to determine whether other URLs are internal or external
    // sets Tag for storage of each page
    public function Root( $URL,$Site )
    {
        $this->RootURL = \fw\URL::Init($URL);
        $this->RootURLStr = \fw\URL::ToString($this->RootURL);

        if( empty($this->RootURL['Hostname']) )
            throw new Exception('Root() URL must contain a domain.');

        $this->RootScheme = $this->RootURL['Scheme'];
        $this->RootDomain = \fw\URL::Hostname($this->RootURL);
        $this->RootPath = \fw\URL::Path($this->RootURL);

        $this->Tag = $this->RootDomain.$this->RootPath;

        $this->Content = $this->Pages = array();

        $J = $this->Juice($this->RootURLStr);

        // do the default directives for the site
        $this->aapi->POST('site_set_directive',array('Name'=>'page','Key'=>'Title','Value'=>$J['Title'],'Site_id'=>$Site['_id']));
        $this->aapi->POST('site_set_directive',array('Name'=>'page','Key'=>'Description','Value'=>$J['Description'],'Site_id'=>$Site['_id']));
        $this->aapi->POST('site_set_directive',array('Name'=>'page','Key'=>'Keywords','Value'=>$J['Keywords'],'Site_id'=>$Site['_id']));

        $this->CreateContentSrv($Site['_id']);

//        $this->ProcessContent($J['Assets']);
//        $this->ProcessPages($J['Links']);

        $this->Screen($J);

        $this->CreatePage('/','Home',$J['Title'],$J['Description'],$J['Keywords'],$J['HTML'],$Site['_id']);

        // obviously this process isn't complete, though it's been difficult to do the diff stuff
        // so we just pull the root and first level links, each as a Base template
        // for each of our "top-level" internal links, juice and screen each page
        foreach( $this->Pages as $P )
        {
            $J2 = $this->Juice(\fw\URL::ToString($P['URL']));

            // $this->ProcessContent($J2['Assets']);
            // we don't crawl secondary links at this point
            // $this->ProcessPages($J2['Links']);

            $this->Screen($J2);

            $this->CreatePage(\fw\Path::ToURLString($P['URL']['Path']),$P['Handle'],$J2['Title'],$J2['Description'],$J2['Keywords'],$J2['HTML'],$Site['_id']);
        }

        // and pull in all content that's been aggregated
        foreach( $this->Content as $C )
        {
            $this->CreateContent($C['URL'],$Site['_id']);
        }
    }

    protected function diff( $Parent,$Child )
    {
        $PFD = fopen('data://text/plain,'.$Parent,'r');
        $CFD = fopen('data://text/plain,'.$Child,'r');

        $FDS = array(
           1 => array("pipe", "w"),
           3 => $PFD,
           4 => $CFD
        );

        $P = proc_open('diff -y -W 200 --left-column  /dev/fd/3 /dev/fd/4',$FDS,$PIPES);

        return stream_get_contents($PIPES[1]);
    }

    protected function normalize_buf( $buf )
    {
        $h = '';
        foreach( explode("\n",str_replace(array("\r","\t"),array('',' '),$buf)) as $V )
        {
            $V = trim($V);
            if( $V === '' )
                continue;
            else
                $h .= trim($V)."\n";
        }

        return $h;
    }

    // standardize the HTML by replace title/desc/keywords, assets, and links with our placeholders and linkers
    protected function Screen( &$Juiced )
    {
        if( !empty($Juiced['Title']) )
        {
            $Title = preg_quote("<title>{$Juiced['Title']}</title>",'/');
            $Juiced['HTML'] = preg_replace("/$Title/",'<title><?=$this($page->Title)?></title>',$Juiced['HTML']);
        }

        if( !empty($Juiced['Description']) )
        {
            $Description = preg_quote("content=\"{$Juiced['Description']}\"",'/');
            $Juiced['HTML'] = preg_replace("/{$Description}/",'content="<?=$this($page->Description)?>"',$Juiced['HTML']);
        }

        if( !empty($Juiced['Keywords']) )
        {
            $Keywords = preg_quote("content=\"{$Juiced['Keywords']}\"",'/');
            $Juiced['HTML'] = preg_replace("/{$Keywords}/",'content="<?=$this($page->Keywords)?>"',$Juiced['HTML']);
        }

        // and replace assets with our linker code
        foreach( $Juiced['Assets'] as $Raw )
        {
            list($Linker,$Handle,$URL) = $this->URLr($Raw,'lc');

            // external link or bogus URL
            if( empty($Linker) )
                continue;

            $Link = preg_quote("\"{$Raw}\"",'/');
            $Juiced['HTML'] = preg_replace("/{$Link}/","\"{$Linker}\"",$Juiced['HTML']);

            $URLStr = \fw\URL::ToString($URL);

            if( !isset($this->Content[$URLStr]) )
                $this->Content[$URLStr] = array('Raw'=>$Raw,'Linker'=>$Linker,'Handle'=>$Handle,'URL'=>$URL);
        }

        // and finally replace the links on the page with our linker code, which will become themselves pages
        foreach( $Juiced['Links'] as $Raw )
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

            if( !isset($this->Pages[$URLStr]) )
                $this->Pages[$URLStr] = array('Raw'=>$Raw,'Linker'=>$Linker,'Handle'=>$Handle,'URL'=>$URL);
        }
    }

    // normalize URL (external/internal based on RootURL) and return proper linker code
    // and replace with proper $Linker ($lp, $lc) - $Linker is just a string
    public function URLr( $OURL,$Linker )
    {
        // leave empty or fragments alone
        if( empty($OURL) || $OURL[0] === '#' )
            return array('','','');

        // leave protocol-relative links alone
        if( $OURL[0] === '/' && $OURL[1] === '/' )
            return array('',$OURL,$OURL);

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
            // pages have an absolute path always
            $URL['Path']['IsAbs'] = TRUE;
            $PageName = pathinfo(\fw\Path::Bottom($URL['Path']),PATHINFO_FILENAME);
            $PageName = strtolower($PageName);

            return array('<?=$lp(\''.$PageName.'\')?>',$PageName,$URL);
        }
        // treat as content - make path not abs/dir and use as content path/handle
        else if( $Linker === 'lc' )
        {
            $URL['Path']['IsDir'] = $URL['Path']['IsAbs'] = FALSE;
            $ContentPath = \fw\Path::ToURLString($URL['Path']);

            return array('<?=$lc(\''.$ContentPath.'\')?>',$ContentPath,$URL);
        }
        else
            throw new Exception("Don't know what $Linker is.");
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
     *
     * @param unknown $URL
     */
    public function Juice( $URL )
    {
        if( empty($this->Tag) )
            throw new Exception('Call Root() first');

        // we ignore others, including content type, since asmblr will handle that
        $Page = array('Title'=>'','Description'=>'','Keywords'=>'',
                      'Assets'=>array(),'Links'=>array(),'HTML'=>'');

        libxml_use_internal_errors(TRUE);
        $DOM = new \DOMDocument;
//         $TR = new \tidy;
//         $TR->parseString($DOM->saveHTML());
//         // var_dump($TR->errorBuffer);
//         $TR->cleanRepair();
// //var_dump((string)$TR);
// //var_dump($DOM->saveHTML());

        $DOM->strictErrorChecking = FALSE;
        $DOM->preserveWhiteSpace = FALSE;
        $DOM->formatOutput = TRUE;
        $DOM->xmlStandalone = TRUE;
        $DOM->recover = TRUE;
        $DOM->resolveExternals = FALSE;
        @$DOM->loadHTML($this->normalize_buf(file_get_contents($URL)));


        $XPR = new \DOMXPath($DOM);

        $R = $XPR->query('//title');
        foreach( $R as $V )
        {
            if( !empty($V->textContent) )
                $Page['Title'] = $V->textContent;
        }

        $R = $XPR->query('//meta[@name="description"]/@content');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Page['Description'] = $V->value;
        }

        $R = $XPR->query('//meta[@name="keywords"]/@content');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Page['Keywords'] = $V->value;
        }

        $R = $XPR->query('//link[@rel="stylesheet"]/@href');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Page['Assets'][] = $V->value;
        }

        $R = $XPR->query('//script/@src');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Page['Assets'][] = $V->value;
        }

        $R = $XPR->query('//a/@href');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Page['Links'][] = $V->value;
        }

        $R = $XPR->query('//img/@src');
        foreach( $R as $V )
        {
            if( !empty($V->value) )
                $Page['Assets'][] = $V->value;
        }

        $Page['HTML'] = $DOM->saveHTML();
        return $Page;
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



// uses DataSet to store directives in the DirectiveSet collection
// this defines the tags for storing the different types of directives (site, page, etc)
// this probably won't let two simultaneous listings of different pages/sites/etc
// consistency checks? prevent DataSet methods from being called?
// this is sort of messy and probably does way too many Refresh() calls
class Directive22Set extends DataSet
{
    protected $SiteTag = 'DirectiveS_';
    protected $PageTag = 'DirectiveP_';


    public function SiteTag( $S )
    {
        $this->ReTag($this->SiteTag.$S['_id']);
        $this->ReSite($S['_id']);
        return $this;
    }
    public function SiteList( $S )
    {
        $this->SiteTag($S);
        return $this;
    }
    public function SiteSet( $S,$D )
    {
        $this->SiteTag($S);
        $this[] = $D;
    }
    public function SiteDelete( $S )
    {
        $this->SiteTag($S);

        // this hack is required because DataSet is doing too many Refresh() etc as part of Delete
        // and also iterating over a DataSet doesn't include _id (just Data) - perhaps this should change
        // same in PageDelete
        foreach( $this->Refresh() as $D )
            $this->Delete($D);
    }

    public function PageTag( $P )
    {
        $this->ReTag($this->PageTag.$P['_id']);
        $this->ReSite($P['Site_id']);
        return $this;
    }
    public function PageList( $P )
    {
        $this->PageTag($P);
        return $this;
    }
    public function PageSet( $P,$D )
    {
        $this->PageTag($P);
        $this[] = $D;
    }
    public function PageDelete( $P )
    {
        $this->PageTag($P);

        foreach( $this->Refresh() as $D )
            $this->Delete($D);
/*
        // this hack is required because DataSet is doing too many Refresh() etc as part of Delete
        $Ds = array();
        foreach( $this as $D )
            $Ds[] = $D;
        foreach( $Ds as $D )
        {
            llog($D);
//            $this->Delete($D);
        }
*/
    }
}




/*
    public static function SitePull()
    {
        if( !fw('page')->LoggedIn )
            fw('lp')->Go('Home');

        static::InitSite(new \MongoId(\fw\Request::Path(-1)));

        fw('page')->ActiveNav = 'Site';
//        fw('html')->Stack('jss_site','jsready');

        fw('html')->ReMap('Base',NULL);
    }
*/


