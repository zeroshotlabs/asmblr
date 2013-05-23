<?php
namespace asm;


// Collection: PageSet
// Unique: DomainPath
abstract class Page extends MongoStruct
{
    protected static $Skel = array('Site_id'=>'','Path'=>'','CurrentTS'=>0,
                                   'Status'=>'Inactive',
                                   'Name'=>'','Routine'=>array(),'Directives'=>array());

    public static function Init( \MongoId $S,$Path )
    {
        $Page = static::$Skel;
        $Page['Site_id'] = $S;
        $T = \fw\Path::Init(strtolower('/'.$Path));
        $Page['Path'] = \fw\Path::ToURLString($T);
        return $Page;
    }
}


// ensureIndex(array('DomainPath'=>1),array('unique'=>TRUE));
// to be updated separately; update search, other dependencies?
// Execute?
class PageSet extends MongoSet
{
    use \fw\PageTraits;


    public function Create( &$P )
    {
        $P['_id'] = new \MongoId;
        $this->MDC->insert($P);
    }

    public function Read( \MongoId $P )
    {
        return $this->MDC->findOne(array('_id'=>$P));
    }

    public function Delete( $P )
    {
        return $this->MDC->remove(array('_id'=>$P['_id'],'CurrentTS'=>$P['CurrentTS']))['n'];
    }

    public function SetStatus( $Status,&$P )
    {
        return $this->SetKV('Status',$Status,'_id',$P);
    }

    public function SetPath( $Path,&$P )
    {
        return $this->SetKV('Path',$Path,'_id',$P);
    }

    public function SetName( $Name,&$P )
    {
        return $this->SetKV('Name',$Name,'_id',$P);
    }

    public function SetRoutine( $Routine,&$P )
    {
        return $this->SetKV('Routine',$Routine,'_id',$P);
    }

    // following three same basically as in SiteSet
    public function SetDirective( $Directive,&$P,\MongoId $D_id = NULL )
    {
        $ds = new DataSet($this->MDB,$P['Site_id'],'DirectiveP_'.$P['_id']);

        // create new directive, append to end
        if( empty($D_id) )
        {
            $ds[] = $Directive;
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
        $ds = new DataSet($this->MDB,$P['Site_id'],'DirectiveP_'.$P['_id']);

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
        $ds = new DataSet($this->MDB,$P['Site_id'],'DirectiveP_'.$P['_id']);

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
        $ds = new DataSet($this->MDB,$SP['Site_id'],'DirectiveP_'.$P['_id']);

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
            llog($O);
            return $ds->Move($D,$O);
        }
    }


    public function Listing( \MongoId $S )
    {
        // $Fields = array('Path'=>1,'Domain'=>1,'Name'=>1,'Status'=>1,'CreateTS'=>1,'UpdateTS'=>1);
        return iterator_to_array($this->MDC->find((array('Site_id'=>$S))));
    }

    // TODO: debugging
    public function Match( \MongoId $S,$Path )
    {
        $T = \fw\Path::Init(strtolower('/'.$Path));
        $DP = \fw\Path::ToURLString($T);

        if( ($P = $this->MDC->findOne(array('Site_id'=>$S,'Path'=>$DP))) !== NULL )
            return ($this->Matched[] = $P);
        else
            return NULL;
    }
}


