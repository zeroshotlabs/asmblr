<?php
namespace asm;



// uses DataSet to store directives in the DirectiveSet collection
// this defines the tags for storing the different types of directives (site, page, etc)
// this probably won't let two simultaneous listings of different pages/sites/etc
// consistency checks? prevent DataSet methods from being called?
class DirectiveSet extends DataSet
{
    protected $SiteTag = 'DirectiveS_';
    protected $PageTag = 'DirectiveP_';

    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id )
    {
        parent::__construct($MDB,$Site_id,$this->SiteTag.$Site_id,TRUE,'DirectiveSet');
    }

    public function SiteTag( $S )
    {
        $this->ReTag($this->SiteTag.$S['_id']);
        $this->ReSite($S['_id']);
        return $this;
    }
    public function PageTag( $P )
    {
        $this->ReTag($this->PageTag.$P['_id']);
        $this->ReSite($P['Site_id']);
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
}




