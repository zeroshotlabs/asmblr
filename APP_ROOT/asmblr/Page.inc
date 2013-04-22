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

    public function SetName( $Name,&$P )
    {
        return $this->SetKV('Name',$Name,'_id',$P);
    }

    public function SetRoutine( $Routine,&$P )
    {
        return $this->SetKV('Routine',$Routine,'_id',$P);
    }

    public function SetDirectives( $Directives,&$P )
    {
        return $this->SetKV('Directives',$Directives,'_id',$P);
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


