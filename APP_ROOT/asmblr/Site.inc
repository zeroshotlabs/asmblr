<?php
namespace asm;

/*
//     protected static $Skel = array('OwningAccount_id'=>'',
//                                   'Users_id'=>array(array('_id'=>'','Level'=>'100')));

    public function Grant( $Email,$Domain,$Level )
    {
        $R = $this->MDB->SiteMeta->update(array('Domain'=>$Domain),array('$addToSet'=>array('Users'=>array('Email'=>$Email,'Level'=>(int)$Level))),array('safe'=>TRUE));
        return $R['n']===1?TRUE:FALSE;
    }

    public function Revoke( $Email,$Domain )
    {
        $this->MDB->SiteMeta->update(array('Domain'=>$Domain),array('$pull'=>array('Users'=>array('Email'=>$Email))));
    }
*/

// Collection: SiteSet
// Unique: Domain
// Directives treated the same way as Page/PageSet
abstract class Site extends MongoStruct
{
    protected static $Skel = array('Domain'=>'','AAID'=>'','CurrentTS'=>0,
                                   'Status'=>'Inactive','BaseURL'=>'',
                                   'Routine'=>array(),'Directives'=>array(),'Config'=>array());

    // do we set BaseURL the same as domain?
    public static function Init( $Domain,\MongoId $AAID )
    {
        $Site = static::$Skel;
        $Site['AAID'] = $AAID;
        static::SetDomain($Domain,$Site);

        return $Site;
    }

    public static function SetDomain( $Domain,&$S )
    {
        $S['Domain'] = strtolower($Domain);
    }

    public static function SetBaseURL( $BaseURL,&$S )
    {
        $S['BaseURL'] = \fw\URL::ToString(\fw\URL::Init($BaseURL));
    }

    // Routine is assumed a string and set as a struct
    public static function SetRoutine( $Routine,&$S )
    {
        if( trim($Routine) === '' )
            $S['Routine'] = array();
        else
            $S['Routine'] = \fw\Routine::Init($Routine);
    }

    // ditto per Page - domain is not saved
    public static function ToPHP( $S )
    {
        $Buf = "<?php\n\n";
        $Buf .= '$Status = '.var_export($S['Status'],TRUE).";\n";
        $Buf .= '$BaseURL = '.var_export($S['BaseURL'],TRUE).";\n\n";

        foreach( $S['Directives'] as $D )
            $Buf .= "\n\$Directives[] = ".var_export($D,TRUE).';';

        // review compatibility with Pointer routines
        $Buf .= "\n\n\n/*** START ROUTINE ***/";
        $Buf .= "\n\$Routine = <<<'_EO_ROUTINE_'\n".\fw\Struct::Get(0,$S['Routine'])."\n\n_EO_ROUTINE_;\n\n";

        return $Buf;
    }

    // see also Page::FromPHP
    public static function FromPHP( $PHP,&$ToSite )
    {
        $Directives = array();
        eval('?>'.$PHP);

        $ToSite = static::Init($ToSite['Domain'],$ToSite['AAID']);

        $ToSite['Status'] = $Status;
        static::SetBaseURL($BaseURL,$ToSite);
        $ToSite['Directives'] = $Directives;
        static::SetRoutine($Routine,$ToSite);
    }
}


// this is different from PageSet as it's only for management + "CRUD"
// of sites... runtime (Match/Execute) happens in asmSite
// ensureIndex(array('Domain'=>1),array('unique'=>TRUE));
class SiteSet extends MongoSet
{
    // Directives need to be saved separately, same as PageSet
    public function Create( &$S )
    {
        $S['_id'] = new \MongoId;
        $S['Directives'] = array();
        $this->MDC->insert($S);
    }

    public function Read( \MongoId $S )
    {
        $S2 = $this->MDC->findOne(array('_id'=>$S));
        if( !empty($S2) )
            $this->Flatten($S2);
        return $S2;
    }

    public function Delete( $S )
    {
        $R = $this->MDC->remove(array('_id'=>$S['_id'],'CurrentTS'=>$S['CurrentTS']))['n'];

        if( !empty($R) )
        {
            $ds = new DirectiveSet($this->MDB,$S['_id']);
            $ds->SiteDelete($S);
        }

        return $R;
    }

    public function SetDomain( $Domain,&$S )
    {
        Site::SetDomain($Domain,$S);
        return $this->SetKV('Domain',$Domain,'_id',$S);
    }

    public function SetStatus( $Status,&$S )
    {
        return $this->SetKV('Status',$Status,'_id',$S);
    }

    // shouldn't we be saving BaseURL as a Path Struct?
    public function SetBaseURL( $BaseURL,&$S )
    {
        Site::SetBaseURL($BaseURL,$S);
        return $this->SetKV('BaseURL',$BaseURL,'_id',$S);
    }

    public function SetRoutine( $Routine,&$S )
    {
        Site::SetRoutine($Routine,$S);
        return $this->SetKV('Routine',$S['Routine'],'_id',$S);
    }

    public function SetDirective( $Directive,&$S,\MongoId $D_id = NULL )
    {
        $ds = new DirectiveSet($this->MDB,$S['_id']);

        // create new directive, append to end
        if( empty($D_id) )
        {
            $ds->SiteSet($S,$Directive);
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

    public function DeleteDirective( \MongoId $D_id,&$S )
    {
        $ds = new DirectiveSet($this->MDB,$S['_id']);

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
    public function CopyDirective( \MongoId $D_id,&$S )
    {
        // why do we specify 'Directive' here?
        $ds = new DirectiveSet($this->MDB,$S['_id'],'Directive');

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
    public function MoveDirective( \MongoId $D_id,\MongoId $NextD_id = NULL,&$S )
    {
        $ds = new DirectiveSet($this->MDB,$S['_id']);

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

    public function SetConfig( $Config,&$S )
    {
        return $this->SetKV('Config',$Config,'_id',$S);
    }

    // each result isn't Flatten()'d
    public function Listing( \MongoId $AAID )
    {
        // $Fields = array('Path'=>1,'Domain'=>1,'Name'=>1,'Status'=>1,'CreateTS'=>1,'UpdateTS'=>1);
        return iterator_to_array($this->MDC->find(array('AAID'=>$AAID)));
    }

    // pull in directives into Directives element
    // when $S is then sent back via JSON, however, Directives gets munged up
    public function Flatten( &$S )
    {
        $ds = new DirectiveSet($this->MDB,$S['_id']);
        $ds->SiteTag($S);

        $S['Directives'] = $ds->SiteList($S);
    }

    public static function MergeSiteArr2WC( $Site,$WCPath )
    {

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

    }

}

