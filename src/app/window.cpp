#include <QHBoxLayout>
#include <QLocale>
#include <QPushButton>

#include "app/quick_views/quick_view.hpp"
#include "app/utilities/icon_engine.hpp"
#include "app/widgets/dialog.hpp"

#include "app/window.hpp"

Dash::NavRail::NavRail()
    : group()
    , timer()
    , layout(new QVBoxLayout())
{
    this->layout->setContentsMargins(0, 0, 0, 0);
    this->layout->setSpacing(0);
}

Dash::Body::Body()
    : layout(new QVBoxLayout())
    , status_bar(new QVBoxLayout())
    , frame(new QStackedLayout())
    , control_bar(new QVBoxLayout())
{
    this->layout->setContentsMargins(0, 0, 0, 0);
    this->layout->setSpacing(0);

    this->status_bar->setContentsMargins(0, 0, 0, 0);
    this->layout->addLayout(this->status_bar);

    this->frame->setContentsMargins(0, 0, 0, 0);
    this->layout->addLayout(this->frame, 1);

    auto msg_ref = new QWidget();
    msg_ref->setObjectName("MsgRef");
    this->layout->addWidget(msg_ref);

    this->control_bar->setContentsMargins(0, 0, 0, 0);
    this->layout->addLayout(this->control_bar);
}

Dash::Dash(Arbiter &arbiter)
    : QWidget()
    , arbiter(arbiter)
    , rail()
    , body()
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addLayout(this->rail.layout);
    layout->addLayout(this->body.layout);

    connect(&this->rail.group, QOverload<int>::of(&QButtonGroup::buttonPressed), [this](int id){
        this->arbiter.set_curr_page(id);
        this->rail.timer.start();
    });
    connect(&this->rail.group, QOverload<int>::of(&QButtonGroup::buttonReleased), [this](int id){
        if (this->rail.timer.hasExpired(1000))
            this->arbiter.set_fullscreen(true);
    });
    connect(&this->arbiter, &Arbiter::curr_page_changed, [this](Page *page){
        this->set_page(page);
    });
    connect(&this->arbiter, &Arbiter::page_changed, [this](Page *page, bool enabled){
        int id = this->arbiter.layout().page_id(page);
        this->rail.group.button(id)->setVisible(enabled);

        if ((this->arbiter.layout().curr_page == page) && !enabled)
            this->arbiter.set_curr_page(this->arbiter.layout().next_enabled_page(page));
    });
}

void Dash::init()
{
    this->body.status_bar->addWidget(this->status_bar());

    for (auto page : this->arbiter.layout().pages()) {
        auto button = page->button();
        button->setCheckable(true);
        button->setFlat(true);
        QIcon icon(new StylizedIconEngine(this->arbiter, QString(":/icons/%1.svg").arg(page->icon_name()), true));
        this->arbiter.forge().iconize(icon, button, 32);

        this->rail.group.addButton(button, this->arbiter.layout().page_id(page));
        this->rail.layout->addWidget(button);
        this->body.frame->addWidget(page->container());

        page->init();
        button->setVisible(page->enabled());
    }
    this->set_page(this->arbiter.layout().curr_page);

    this->body.control_bar->addWidget(this->control_bar());
}

void Dash::set_page(Page *page)
{
    auto id = this->arbiter.layout().page_id(page);
    this->rail.group.button(id)->setChecked(true);
    this->body.frame->setCurrentWidget(page->container());
}

QWidget *Dash::status_bar() const
{
    auto widget = new QWidget();
    widget->setObjectName("StatusBar");
    auto layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    auto clock = new QLabel();
    clock->setFont(this->arbiter.forge().font(15, true));
    clock->setAlignment(Qt::AlignCenter);
    layout->addWidget(clock);

    /*
     * new
     */
    //Climate climate = new Climate(*this->arbiter);
    auto test = new QLabel();
    test->setFont(this->arbiter.forge().font(15, true));
    test->setAlignment(Qt::AlignLeft);
    layout->addWidget(test);
	test->setText("Hier Temparatur von Clima");

    connect(&this->arbiter.system().clock, &Clock::ticked, [clock](QTime time){
        clock->setText(QLocale().toString(time, QLocale::ShortFormat));
    });
/*
    widget->setVisible(this->arbiter.layout().status_bar);
    connect(&this->arbiter, &Arbiter::status_bar_changed, [widget](bool enabled){
        widget->setVisible(enabled);
    });
*/
    return widget;
}

QWidget *Dash::control_bar() const
{
    auto widget = new QWidget();
    widget->setObjectName("ControlBar");
    auto layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto quick_views = new QStackedLayout();
    quick_views->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(quick_views);
    for (auto quick_view : this->arbiter.layout().control_bar.quick_views()) {
        quick_views->addWidget(quick_view->widget());
        quick_view->init();
    }
    quick_views->setCurrentWidget(this->arbiter.layout().control_bar.curr_quick_view->widget());
    connect(&this->arbiter, &Arbiter::curr_quick_view_changed, [quick_views](QuickView *quick_view){
        quick_views->setCurrentWidget(quick_view->widget());
    });

    layout->addStretch();

    auto dialog = new Dialog(this->arbiter, true, this->arbiter.window());
    dialog->set_title("Power Off");
    dialog->set_body(this->power_control());
    auto shutdown = new QPushButton();
    shutdown->setFlat(true);
    this->arbiter.forge().iconize("power_settings_new", shutdown, 26);
    layout->addWidget(shutdown);
    connect(shutdown, &QPushButton::clicked, [dialog]{ dialog->open(); });

    auto exit = new QPushButton();
    exit->setFlat(true);
    this->arbiter.forge().iconize("close", exit, 26);
    layout->addWidget(exit);
    connect(exit, &QPushButton::clicked, []{ qApp->exit(); });

    widget->setVisible(this->arbiter.layout().control_bar.enabled);
    connect(&this->arbiter, &Arbiter::control_bar_changed, [widget](bool enabled){
        widget->setVisible(enabled);
    });

    return widget;
}

QWidget *Dash::power_control() const
{
    auto widget = new QWidget();
    auto layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto restart = new QPushButton();
    restart->setFlat(true);
    this->arbiter.forge().iconize("refresh", restart, 36);
    connect(restart, &QPushButton::clicked, [this]{
        this->arbiter.settings().sync();
        sync();
        system(Session::System::REBOOT_CMD);
    });
    layout->addWidget(restart);

    auto power_off = new QPushButton();
    power_off->setFlat(true);
    this->arbiter.forge().iconize("power_settings_new", power_off, 36);
    connect(power_off, &QPushButton::clicked, [this]{
        this->arbiter.settings().sync();
        sync();
        system(Session::System::SHUTDOWN_CMD);
    });
    layout->addWidget(power_off);

    return widget;
}

MainWindow::MainWindow(QRect geometry)
    : QMainWindow()
    , arbiter(this->init(geometry))
    , stack(new QStackedWidget())
{
    this->setAttribute(Qt::WA_TranslucentBackground, true);

    auto frame = new QFrame();
    auto layout = new QVBoxLayout(frame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(this->stack);
    layout->addWidget(this->arbiter.layout().fullscreen.toggler(1)->widget());

    this->setCentralWidget(frame);

    auto dash = new Dash(this->arbiter);
    this->stack->addWidget(dash);
    dash->init();

    this->arbiter.system().brightness.set();

    if (this->arbiter.layout().fullscreen.on_start)
        this->arbiter.set_fullscreen(true);
}

MainWindow *MainWindow::init(QRect geometry)
{
    // force to either screen or custom size
    this->setFixedSize(geometry.size());
    this->move(geometry.topLeft());

    return this;
}

void MainWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    this->arbiter.update();
}

void MainWindow::set_fullscreen(Page *page)
{
    auto widget = page->container()->take();
    this->stack->addWidget(widget);
    this->stack->setCurrentWidget(widget);
}
