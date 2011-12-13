(function ($) {
  Drupal.behaviors.flagListsOps = {
    attach: function(context) {
      // Hide the go button, as it is not needed for JS enabled browsers.
      $('.flag-lists-ops-go', context).hide();

      // Make select all checkbox work
      $('input.flo-table-select-all', context).each(function(i) {
        var selectall = $(this);

        if (!selectall.hasClass('processed')) {
          selectall.change(function(e) {
            $('input.flo-select', $(this).parents('form')).attr('checked', $(this).attr('checked'));
          }).addClass('processed');
        }
      });

      // Animate the deletion for AJAX deleting.
      $('.flo-deleted-value', context).each(function(i) {
        var parent = $(this).parents('.view');
        $('.flo-select[value='+$(this).val()+']', parent).each(function(i) {
          $(this).parents('.views-row, tr').fadeOut();
        });
      });
    }
  }
})(jQuery);
