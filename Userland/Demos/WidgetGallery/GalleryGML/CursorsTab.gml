@GUI::Widget {
    name: "cursors_tab"
    layout: @GUI::VerticalBoxLayout {
        margins: [4]
    }

    @GUI::GroupBox {
        layout: @GUI::VerticalBoxLayout {
            margins: [8]
        }

        @GUI::TableView {
            name: "cursors_tableview"
            font_size: 12
        }
    }
}
