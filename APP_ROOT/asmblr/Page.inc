<?php
namespace asm;


// Collection: PageSet
// Unique: Site_id/Name, Site_id/Path  (see REST.inc for definitve)
// The Directives element is stored empty and only populated from DataSet upon Match/Read
abstract class Page extends MongoStruct
{
    protected static $Skel = array('Site_id'=>'','Path'=>'','Name'=>'',
                                   'CurrentTS'=>0,'Status'=>'Inactive',
                                   'PathStruct'=>array(),'Routine'=>array(),'Directives'=>array());

    public static function Init( \MongoId $S,$Name,$Path )
    {
        $Page = static::$Skel;
        $Page['Site_id'] = $S;
        $Page['Name'] = $Name;
        static::SetPath($Path,$Page);

        return $Page;
    }

    // create a standardized name based on the path
    // note that this should be used only for automatically generated/imported pages
    // expects a Path struct
    public static function Path2Name( $Path )
    {
        // might cause issues but needed in most cases it seems
        if( $Path['IsRoot'] === TRUE )
            return 'ROOT';

        $Segments = array();
        foreach( $Path['Segments'] as $S )
            $Segments[] = pathinfo($S,PATHINFO_FILENAME);

        return strtolower(implode('_',$Segments));
    }

    public static function SetPath( $Path,&$P )
    {
        $P['PathStruct'] = \fw\Path::Init(strtolower('/'.$Path));
        $P['Path'] = \fw\Path::ToURLString($P['PathStruct']);
    }

    // Routine is assumed a string and set as a struct
    // potentially a pointer routine could be mis-interpreted (security issue)
    public static function SetRoutine( $Routine,&$P )
    {
        if( trim($Routine) === '' )
            $P['Routine'] = array();
        else
            $P['Routine'] = \fw\Routine::Init($Routine);
    }

    // converts a Page Struct to a string (PHP code), suitable for saving/editing as a file
    // the Page Name is NOT saved, since the file name dictates that
    // Directives is assumed to be PageSet::Flatten()'d already
    public static function ToPHP( $P )
    {
        $Buf = "<?php\n\n";
        $Buf .= '$Status = '.var_export($P['Status'],TRUE).";\n";
        $Buf .= '$Path = '.var_export($P['Path'],TRUE).";\n\n";

        foreach( $P['Directives'] as $D )
            $Buf .= "\n\$Directives[] = ".var_export($D,TRUE).';';

        // review compatibility with Pointer routines
        $Buf .= "\n\n\n/*** START ROUTINE ***/";
        $Buf .= "\n\$Routine = <<<'_EO_ROUTINE_'\n".\fw\Struct::Get(0,$P['Routine'])."\n\n_EO_ROUTINE_;\n\n";

        return $Buf;
    }

    // probably should have some type of security and validity checks about what we end up using
    // actually should probably use get_defined_vars() and use our form validators....
    // doesn't actually save the directives to that has to be done on it's own.
    public static function FromPHP( $PHP,&$ToPage )
    {
        $Directives = array();
        eval('?>'.$PHP);

        $ToPage = static::Init($ToPage['Site_id'],$ToPage['Name'],$Path);

        $ToPage['Status'] = $Status;
        $ToPage['Directives'] = $Directives;

        static::SetRoutine($Routine,$ToPage);
    }
}


// ensureIndex(array('DomainPath'=>1),array('unique'=>TRUE));
// to be updated separately; update search, other dependencies?
// Execute?
class PageSet extends MongoSet
{
    use \fw\PageTraits;

    protected $Site_id;
    // name => path
    protected $PageMap = array();


    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id = NULL,$Collection = NULL )
    {
        parent::__construct($MDB,$Collection);

        if( !empty($Site_id) )
        {
            $this->Load($Site_id);
            $this->Site_id = $Site_id;
        }
    }

    // debately - also see TemplateSet::Load()
    // currently this is only for LinkPage purposes thus only paths/names are pulled
    // however, maybe also good for Match() if we had path => name map?
    public function Load( \MongoId $S )
    {
        $this->PageMap = array();

        // wasteful - should be doing a better query than a full pull
        foreach( $this->Listing($S) as $V )
            $this->PageMap[$V['Name']] = $V['Path'];
    }

    public function Create( &$P )
    {
        $P['_id'] = new \MongoId;
        // Directives should always be stored in DirectiveSet
        $P['Directives'] = array();
        $this->MDC->insert($P);
        $this->PageMap[$P['Name']] = $P['Path'];
    }

    // so we have two modes - tied to a Site_id or not - if not, this method won't work
    // in asmSrv we're in Site_id mode - in console we typically aren't
    // the rest of the CRUD methods work by Page _id so they always work and can reach
    // across sites
    public function Read( $Name )
    {
        $P = $this->MDC->findOne(array('Site_id'=>$this->Site_id,'Name'=>$Name));
        if( !empty($P) )
            $this->Flatten($P);
        return $P;
    }

    public function Read_id( \MongoId $P )
    {
        $P2 = $this->MDC->findOne(array('_id'=>$P));
        if( !empty($P2) )
            $this->Flatten($P2);
        return $P2;
    }

    public function Delete( $P )
    {
        $R = $this->MDC->remove(array('_id'=>$P['_id'],'CurrentTS'=>$P['CurrentTS']))['n'];

        if( !empty($R) )
        {
            $ds = new DirectiveSet($this->MDB,$P['Site_id']);
            $ds->PageDelete($P);
        }

        return $R;
    }

    public function SetStatus( $Status,&$P )
    {
        return $this->SetKV('Status',$Status,'_id',$P);
    }

    public function SetName( $Name,&$P )
    {
        return $this->SetKV('Name',$Name,'_id',$P);
    }

    // could use some more validation or something maybe
    public function SetPath( $Path,&$P )
    {
        Page::SetPath($Path,$P);
        return $this->SetKV('Path',$P['Path'],'_id',$P) && $this->SetKV('PathStruct',$P['PathStruct'],'_id',$P);
    }

    public function SetRoutine( $Routine,&$P )
    {
        Page::SetRoutine($Routine,$P);
        return $this->SetKV('Routine',$P['Routine'],'_id',$P);
    }

    // following three same basically as in SiteSet
    public function SetDirective( $Directive,&$P,\MongoId $D_id = NULL )
    {
        $ds = new DirectiveSet($this->MDB,$P['Site_id']);
        $ds->PageTag($P);

        // create new directive, append to end
        if( empty($D_id) )
        {
            $ds->PageSet($P,$Directive);
            return TRUE;
        }
        // update existing directive
        else
        {
            $D = $ds->Read($D_id);
            if( empty($D) )
                return FALSE;

            $ds->SetData($Directive,$D);
            return TRUE;
        }
    }

    public function DeleteDirective( \MongoId $D_id,&$P )
    {
        $ds = new DirectiveSet($this->MDB,$P['Site_id']);
        $ds->PageTag($P);

        // good example of when we institute CurrentTS checking, we couldn't do this
        // as the Data struct would need to be passed to us
        $D = $ds->Read($D_id);
        if( empty($D) )
            return FALSE;
        else
        {
            $ds->Delete($D);
            return TRUE;
        }
    }

    // different - returns NULL or the new Data struct
    public function CopyDirective( \MongoId $D_id,&$P )
    {
        $ds = new DirectiveSet($this->MDB,$P['Site_id']);
        $ds->PageTag($P);

        $D = $ds->Read($D_id);
        if( empty($D) )
        {
            return NULL;
        }
        else
        {
            $NewO = $ds->OOf($D['_id']);
            return $ds->Move($D,$NewO+1,TRUE);
        }
    }

    // different - returns NULL or the new Data struct
    public function MoveDirective( \MongoId $D_id,\MongoId $NextD_id = NULL,&$P )
    {
        $ds = new DirectiveSet($this->MDB,$P['Site_id']);
        // forgetting to do this results in nasty bugs - perhaps we find a way to prevent it
        // maybe a site_id isn't part of the constructor so a site or page has to be set?
        $ds->PageTag($P);

        $D = $ds->Read($D_id);
        if( empty($D) )
            return NULL;

        // move to end
        if( empty($NextD_id) )
        {
            return $ds->Move($D,NULL);
        }
        else
        {
            $O = $ds->OOf($NextD_id);
            return $ds->Move($D,$O);
        }
    }

    // each result isn't Flatten()'d
    public function Listing( \MongoId $S )
    {
        // $Fields = array('Path'=>1,'Domain'=>1,'Name'=>1,'Status'=>1,'CreateTS'=>1,'UpdateTS'=>1);
        return iterator_to_array($this->MDC->find((array('Site_id'=>$S)))->sort(array('Name'=>TRUE)));
    }

    // TODO: debugging
    public function Match( \MongoId $S,$Path )
    {
        $T = \fw\Path::Init(strtolower('/'.$Path));
        $DP = \fw\Path::ToURLString($T);

        if( ($P = $this->MDC->findOne(array('Site_id'=>$S,'Path'=>$DP))) !== NULL )
        {
            $this->Flatten($P);
            return ($this->Matched[] = $P);
        }
        else
            return NULL;
    }

    // pull in directives into Directives element
    public function Flatten( &$P )
    {
        $ds = new DirectiveSet($this->MDB,$P['Site_id']);
        $ds->PageTag($P);

        $P['Directives'] = $ds->PageList($P);
    }
}


