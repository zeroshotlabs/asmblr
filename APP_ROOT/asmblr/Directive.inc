<?php
namespace asm;



// uses DataSet to store directives in the DirectiveSet collection
// this defines the tags for storing the different types of directives (site, page, etc)
// this probably won't let two simultaneous listings of different pages/sites/etc
class DirectiveSet extends DataSet
{
    protected $SiteTag = 'DirectiveS_';
    protected $PageTag = 'DirectiveP_';

    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id )
    {
        parent::__construct($MDB,$Site_id,$this->SiteTag.$Site_id,TRUE,'DirectiveSet');
    }

    public function SiteList( $S )
    {
        $this->ReTag($this->SiteTag.$S['_id']);
        $this->ReSite($S['_id']);
        return $this;
    }
    public function SiteSet( $S,$D )
    {
        $this->ReTag($this->SiteTag.$S['_id']);
        $this->ReSite($S['_id']);
        $this[] = $D;
    }
    public function PageList( $P )
    {
        $this->ReTag($this->PageTag.$P['_id']);
        $this->ReSite($P['Site_id']);
        return $this;
    }
    public function PageSet( $P,$D )
    {
        $this->ReTag($this->PageTag.$P['_id']);
        $this->ReSite($P['Site_id']);
        $this[] = $D;
    }
}




