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

    // do we set BaseURL the same as domain?
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

    // shouldn't we be saving BaseURL as a Path Struct?
    public function SetBaseURL( $BaseURL,&$S )
    {
        return $this->SetKV('BaseURL',$BaseURL,'_id',$S);
    }

    public function SetRoutine( $Routine,&$S )
    {
        return $this->SetKV('Routine',$Routine,'_id',$S);
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

    public function Listing( \MongoId $AAID )
    {
        // $Fields = array('Path'=>1,'Domain'=>1,'Name'=>1,'Status'=>1,'CreateTS'=>1,'UpdateTS'=>1);
        return iterator_to_array($this->MDC->find(array('AAID'=>$AAID)));
    }
}

