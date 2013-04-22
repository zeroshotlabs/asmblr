<?php
namespace asm;

//     protected static $Skel = array('OwningAccount_id'=>'',
//                                   'Users_id'=>array(array('_id'=>'','Level'=>'100')));


// Collection: SiteSet
// Unique: Domain
abstract class Site extends MongoStruct
{
    protected static $Skel = array('Domain'=>'','AAID'=>'','CurrentTS'=>0,
                                   'Status'=>'Inactive','BaseURL'=>'',
                                   'Routine'=>array(),'Directives'=>array(),'Config'=>array());

    public static function Init( $Domain,\MongoId $AAID )
    {
        $Site = static::$Skel;
        $Site['Domain'] = strtolower($Domain);
        $Site['AAID'] = $AAID;
        return $Site;
    }
}


// this is different from PageSet as it's only for management + "CRUD"
// of sites... runtime (Match/Execute) happens in asmSite
// ensureIndex(array('Domain'=>1),array('unique'=>TRUE));
class SiteSet extends MongoSet
{
    public function Create( &$S )
    {
        $S['_id'] = new \MongoId;
        $this->MDC->insert($S);
    }

    public function Read( \MongoId $S )
    {
        return $this->MDC->findOne(array('_id'=>$S));
    }

    public function Delete( $S )
    {
        return $this->MDC->remove(array('_id'=>$S['_id'],'CurrentTS'=>$S['CurrentTS']))['n'];
    }

    public function SetDomain( $Domain,&$S )
    {
        return $this->SetKV('Domain',$Domain,'_id',$S);
    }

    public function SetStatus( $Status,&$S )
    {
        return $this->SetKV('Status',$Status,'_id',$S);
    }

    public function SetBaseURL( $BaseURL,&$S )
    {
        return $this->SetKV('BaseURL',$BaseURL,'_id',$S);
    }

    public function SetRoutine( $Routine,&$S )
    {
        return $this->SetKV('Routine',$Routine,'_id',$S);
    }

    public function SetDirectives( $Directives,&$S )
    {
        return $this->SetKV('Directives',$Directives,'_id',$S);
    }

    public function SetConfig( $Config,&$S )
    {
        return $this->SetKV('Config',$Config,'_id',$S);
    }

    public function Listing( \MongoId $AAID )
    {
        // $Fields = array('Path'=>1,'Domain'=>1,'Name'=>1,'Status'=>1,'CreateTS'=>1,'UpdateTS'=>1);
        return iterator_to_array($this->MDC->find(array('AAID'=>$AAID)));
    }
}

