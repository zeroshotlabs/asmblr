
 // validate: function(v){return '';}

    $(document).ajaxStart(function() { $('#spinner').show(); });
    $(document).ajaxComplete(function() { window.setTimeout(function() { $('#spinner').hide(); },200)});

    headers = {};
   <?php if( $page->LoggedIn ): ?>
    headers = {'X-ASMBLR-USER': '<?=$_SESSION['Account']['_id']?>',
 	             'X-ASMBLR-PW': '<?=$_SESSION['Account']['Password']?>'};
   <?php endif; ?>

    ActiveNav = '<?=$page->ActiveNav?>';
    
    asmblr_ids = {};
   <?php if( $page->ActiveNav === 'Site' ): ?>
    asmblr_ids['Site_id'] = '<?=$S['_id']?>';
    Type = 'site';
   <?php elseif( $page->ActiveNav === 'Page' ): ?>
    asmblr_ids['Page_id'] = '<?=$P['_id']?>';
    asmblr_ids['Site_id'] = '<?=$S['_id']?>';
    Type = 'page';
   <?php elseif( $page->ActiveNav === 'Template' ): ?>
    asmblr_ids['Template_id'] = '<?=$T['_id']?>';
    asmblr_ids['Site_id'] = '<?=$S['_id']?>';
    Type = 'template';
   <?php elseif( $page->ActiveNav === 'Content' ): ?>
    asmblr_ids['Content_id'] = '<?=$C['_id']?>';
    asmblr_ids['Site_id'] = '<?=$S['_id']?>';
    Type = 'content';
   <?php elseif( $page->ActiveNav === 'ContentUpload' ): ?>
    asmblr_ids['Site_id'] = '<?=$S['_id']?>';
    Type = 'content';
   <?php else: ?>
    Type = '';
   <?php endif; ?>

    $.ajaxSetup({headers:headers,data:asmblr_ids,dataType:'json',type:'POST'});

	// setup editable defaults
    // upon click to edit we could pull the latest value and compare timestamps/values and
    // notify user if it's changed; then push up the latest timestamp we have; of course this
    // ideally means we store a timestamp for every mongo field, not just the whole document
	$.fn.editable.defaults.mode = 'popup';
	$.fn.editable.defaults.onblur = 'ignore';
	$.fn.editable.defaults.send = 'always';
	$.fn.editable.defaults.placement = 'right';
	$.fn.editable.defaults.success = function(r,nv){ if( r.Status === false ) return r.Msg; };
	$.fn.editable.defaults.validate = function(v){if($.trim(v)=='') return 'Required.';};
	$.fn.editable.defaults.params = function(p){return xedit_norm(p)};

	// set up directives - this is going to fire a lot of AAPI requests but for now so what
	// since our parent can be called multiple times, will this be a problem and stackup?
	$('#dir-container').on('change','.dir-part',function(e){
		pk = $(e.currentTarget).data('pk');

		// work on storing a new directive - wait until we have all three data points then store
		// and Javascript sucks
		if( pk === '' )
		{
			d = {};
			i = 0;
			$('input.dir-part,select.dir-part','#new-directive').each(function(k,v){
				v2 = $(v);
				if( v2.val().trim() !== '' )
				{
				    d[v2[0].name] = v2.val();
				    ++i;
				}
			});

			if( i === 3 )
			{
				aapi_set(Type+'_set_directive',d);
				ajf_directive_grid(Type);
			}
		}
		else
		{
        	d = {D_id:pk};
        	done = false;
    		// update existing, including copy or delete
    		$('input.dir-part,select.dir-part','#'+pk).each(function(k,v){
    			v2 = $(v);

    			if( v2.val() === 'delete' )
    			{
    				$.ajax({ url:aapi_method2url(Type+'_del_directive'),data:d,
    					success: function(data){ $('#'+pk).fadeOut(100,function(){ $('#'+pk).remove();})}})
    				.fail(function(){ console.log('connection error');});
    				done = true;
    			}
    			else if( v2.val() === 'copy' )
    			{
    				$.ajax({ url:aapi_method2url(Type+'_cp_directive'),data:d,
    					success:function(data){ ajf_directive_grid(Type); }})
    				.fail(function(){console.log('connection error');});
    				done = true;
    			}
    			// will end up updating
    			else if( done === false )
    			    d[v2[0].name] = v2.val();
    		});

    		if( done === false )
    		    aapi_set(Type+'_set_directive',d);
		}
	});
