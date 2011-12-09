(function ($) {
  Drupal.behaviors.flagListsOps = {
    attach: function() {
      // Hide the go button, as it is not needed for JS enabled browsers.
      $('.flag-lists-ops-go').hide();

      // Make select all checkbox work
      $('input.flo-table-select-all').each(function(i) {
        var selectall = $(this);

        if (!selectall.hasClass('processed')) {
          selectall.change(function(e) {
            $('input.flo-select', $(this).parents('form')).attr('checked', $(this).attr('checked'));
          }).addClass('processed');
        }
      });
    }
  }
})(jQuery);
