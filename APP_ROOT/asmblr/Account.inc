<?php
namespace asm;


// Collection: Account
// Unique: Email
abstract class Account extends MongoStruct
{
    protected static $Skel = array('CurrentTS'=>0,'Email'=>'',
                                   'Status'=>'Active','Password'=>'','Name'=>'',
                                   'Company'=>'','Billing'=>array());

    public static function Init( $Email,$Password,$Name,$Company = '' )
    {
        $Account = static::$Skel;
        $Account['Email'] = strtolower($Email);
        $Account['Password'] = md5($Password);
        $Account['Name'] = $Name;
        $Account['Company'] = $Company;
        return $Account;
    }
}



// ensureIndex(array('Email'=>1),array('unique'=>TRUE));
// to be updated separately; update search, other dependencies?
class AccountSet extends MongoSet
{
    public function Create( &$A )
    {
        $this->MDC->insert($A);
    }

    public function Read( \MongoId $A )
    {
        return $this->MDC->findOne(array('_id'=>$A));
    }

    public function Delete( $A )
    {
        return $this->MDC->remove(array('_id'=>$A['_id'],'CurrentTS'=>$A['CurrentTS']))['n'];
    }

    public function SetStatus( $Status,&$A )
    {
        return $this->SetKV('Status',$Status,'_id',$A);
    }

    public function SetPassword( $Password,&$A )
    {
        return $this->SetKV('Password',md5($Password),'_id',$A);
    }

    public function SetName( $Name,&$A )
    {
        return $this->SetKV('Name',$Name,'_id',$A);
    }

    public function SetCompany( $Company,&$A )
    {
        return $this->SetKV('Company',$Company,'Email',$A);
    }

    public function Auth( $Email,$Password )
    {
        return $this->MDC->findOne(array('Email'=>$Email,'Password'=>md5($Password)));
    }
}

