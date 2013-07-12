<?php
namespace asm;



// uses DataSet to store directives in the DirectiveSet collection
// this defines the tags for storing the different types of directives (site, page, etc)
// this probably won't let two simultaneous listings of different pages/sites/etc
// consistency checks? prevent DataSet methods from being called?
// this is sort of messy and probably does way too many Refresh() calls
class DirectiveSet extends DataSet
{
    protected $SiteTag = 'DirectiveS_';
    protected $PageTag = 'DirectiveP_';

    public function __construct( \fw\MongoDB $MDB,\MongoId $Site_id )
    {
        parent::__construct($MDB,$Site_id,$this->SiteTag.$Site_id,'DirectiveSet');
    }

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




