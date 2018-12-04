if (THIS_USER_IS_NEW) {
    $.ajax('new-user-presentation.html').done(function (res) {
        let curIndex = 0;
        let elems = $(res.documentElement).children();

        function show(i) {
            if (i == elems.length)
                return;

            swal({
                content: elems[i],
                buttons: {
                    next: {
                        text: 'Next',
                        value: 'next'
                    },
                    cancel: 'Skip'
                },
                closeOnClickOutside: false
            }).then(function(val) {
                if (!val)
                    return;

                show(i + 1);
            });
        }

        show(0);
    });
}