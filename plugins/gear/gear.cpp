/*****************************************************************************/
/*  gear.cpp - plugin gear for LibreCAD                                      */
/*                                                                           */
/*  Copyright (C) 2016 Cédric Bosdonnat cedric@bosdonnat.fr                  */
/*  Edited 2017 Luis Colorado <luiscoloradourcola@gmail.com>                 */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#include <QGridLayout>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>
#include <QTransform>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <vector>
#include <cmath>

#include "document_interface.h"
#include "gear.h"

#define F(str) __FILE__":%d:%s: " str, __LINE__, __func__
#define I(str) "INFO: " str
#define INFO(str, ...) do{fprintf(stderr, F(I(str)), ##__VA_ARGS__); fflush(stderr);}while(0)
#define P(exp) INFO("%20s: %.10lg\n", #exp, (double)(exp))

QString LC_Gear::name() const
{
    return (tr("Gear creation plugin"));
}

PluginCapabilities LC_Gear::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("Gear plugin"));
    return pluginCapabilities;
}

LC_Gear::LC_Gear():parameters_dialog(0)
{
}

LC_Gear::~LC_Gear()
{
}

void LC_Gear::execComm(Document_Interface *doc,
                        QWidget *parent, QString cmd)
{
    Q_UNUSED(doc);
    Q_UNUSED(cmd);

    QPointF center;
    if (!doc->getPoint(&center, QString("select center"))) {
        INFO("NO SELECTED CENTER, ABANDON\n");
        return;
    }

    INFO("Selected center is at [%12.8f,%12.8f]\n", center.x(), center.y());

    if (!parameters_dialog) {
        INFO("CREATING parameters_dialog\n");
        parameters_dialog = new lc_Geardlg(parent);
        if (!parameters_dialog) {
            INFO("Cannot create parameters_dialog\n");
            return;
        }
        INFO("parameters_dialog == %p\n", parameters_dialog);
    }

    int result =  parameters_dialog->exec();
    if (result == QDialog::Accepted)
        parameters_dialog->processAction(doc, cmd, center);
}

/*****************************/

lc_Geardlg::lc_Geardlg(QWidget *parent) :
    QDialog(parent),
    settings(QSettings::IniFormat, QSettings::UserScope, "LibreCAD", "gear_plugin")
{
    const char *windowTitle = "Draw a gear";

    INFO("SET WINDOW TITLE \"%s\"\n", windowTitle);

    setWindowTitle(tr(windowTitle));

    QLabel *label;
    QGridLayout *mainLayout = new QGridLayout(this);

    int i = 0, j = 0;

#define RST() do{ if(j) { ++i; j = 0; } } while(0)

#define Q(name, type, text, min, max, stp) do {                  \
            INFO("Creating " #type " " #name " at dialog row %d\n", i); \
            label = new QLabel((text), this);                    \
            name = new type(this);                               \
            name->setMinimum(min);                               \
            name->setMaximum(max);                               \
            name->setSingleStep(stp);                            \
            mainLayout->addWidget(label,  i, 0);                 \
            mainLayout->addWidget((name), i, 1);                 \
        } while(0)

#define QDSB(name, text, min, max, stp, dec) do {                \
            RST();                                               \
            Q(name, QDoubleSpinBox, (text),(min),(max),(stp));   \
            name->setDecimals(dec);                              \
            ++i; j = 0;                                          \
        } while(0)

#define QSB(name, text, min, max, stp) do {                      \
            RST();                                               \
            Q(name, QSpinBox, (text),(min),(max), (stp));        \
            ++i; j = 0;                                          \
        } while(0)

#define QCB(name, text) do {                                     \
            INFO("Creating QCheckBox " #name " at dialog position (%d, %d)\n", i, j); \
            name = new QCheckBox((text), this);                   \
            mainLayout->addWidget(name, i, j);                   \
            j++; if (j >= 2) { j = 0; i++; }     \
        } while(0)

    QDSB(rotateBox,             tr("Rotation angle"),                        -360.0,      360.0,     1.0, 6);
    QSB (nteethBox,             tr("Number of teeth"),                          1,       2000,       1);
    QDSB(modulusBox,            tr("Modulus"),                                  1.0E-10,    1.0E+10, 0.1, 6); 
    QDSB(pressureBox,           tr("Pressure angle (deg)"),                     0.1,       89.9,     1.0, 5);
    QDSB(addendumBox,           tr("Addendum (rel. to modulus)"),               0.0,        5.0,     0.1, 5);
    QDSB(dedendumBox,           tr("Dedendum (rel. to modulus)"),               0.0,        5.0,     0.1, 5);
    QSB (n1Box,                 tr("Number of segments to draw (dedendum)"),    1,       1024,       8);
    QSB (n2Box,                 tr("Number of segments to draw (addendum)"),    1,       1024,       8);

    QCB (useLayersBox,          tr("Use layers?")); RST();
    QCB (drawAddendumCircleBox, tr("Draw addendum circle?"));
    QCB (drawPitchCircleBox,    tr("Draw pitch circle?"));
    QCB (drawBaseCircleBox,     tr("Draw base circle?"));
    QCB (drawRootCircleBox,     tr("Draw root circle?"));
    QCB (drawPressureLineBox,   tr("Draw pressure line?"));
    QCB (drawPressureLimitBox,  tr("Draw pressure limits?"));

    QCB (calcInterferenceBox,   tr("Calculate interference?"));
    QSB (n3Box,                 tr("Number of segments to draw (interference)"), 1, 1024,     8);

    QPushButton *acceptbut = new QPushButton(tr("Accept"), this);
    QPushButton *cancelbut = new QPushButton(tr("Cancel"), this);
    QHBoxLayout *acceptLayout = new QHBoxLayout();

    acceptLayout->addStretch();
    acceptLayout->addWidget(acceptbut);
    acceptLayout->addStretch();
    acceptLayout->addWidget(cancelbut);
    acceptLayout->addStretch();

    mainLayout->addLayout(acceptLayout, i, 0, 1, 2);
    setLayout(mainLayout);

    readSettings();

    connect(cancelbut, SIGNAL(clicked()), this, SLOT(reject()));
    connect(acceptbut, SIGNAL(clicked()), this, SLOT(checkAccept()));
    INFO("END\n");
}

/* calculate the radius of a point in canonical evoluta
 * whose radius is given. */
static double radius2arg(const double radius, const double alpha = 0.0)
{
    const double aux = 1.0 - alpha;
    return sqrt(radius * radius - aux*aux);
}

/* canonical evolute is generated by a 1.0 radius circle.
 * We consider it the next complex function:
 * (1.0 - alpha - i*phi) * exp(i*phi) 
 */
static double re_evolute(const double phi, const double alpha = 0.0)
{
    return (1.0 - alpha) * cos(phi) + phi * sin(phi);
}

static double im_evolute(const double phi, const double alpha = 0.0)
{
    return (1.0 - alpha) * sin(phi) - phi * cos(phi);
}

#if 0
/* modulus of evolute at point phi */
static double mod_evolute(const double phi, const double alpha = 0.0)
{
    const double aux = 1.0 - alpha;
    return sqrt(aux*aux + phi*phi);
}

/* finds the root of a function given two points.  The algorithm treats to follow the line
 * that joins the two images of the points a and b, until it crosses the X axis.  Then the
 * function is evaluated on the found point and this value is used to substitute and discard
 * the farthest of the values we had previously.  Once the values are within the epsilon
 * parameter supplied (or defaulted to 1.0e-13) the algorithm terminates.  This is expected
 * to work as the value to be found is for a function that is warranted to have a zero and
 */
static double find_root(const evolute& function, double a, double b, const double eps = 1.0e-13)
{
    double f_a = function(a), f_b = function(b);
    do {
        const double x = (a*f_b - b*f_a) / (f_b - f_a), f_x = function(x);
        if (fabs(x - a) < fabs(x - b)) {
            b = x; f_b = f_x;
        } else {
            a = x; f_a = f_x;
        }
    } while (fabs(a-b) > eps);
    return a;
} /* find_root */

#endif

struct evolute {
    evolute(int n_t, double add, double ded, double p_ang);

    const int n_teeth;
    const double
        addendum, dedendum,
        c_modulus,
        p_angle, cos_p_angle,
        off_rot, cos_off_rot, sin_off_rot,
        dedendum_radius, addendum_radius,
        phi_at_dedendum, phi_at_addendum,
        alpha, angle_0;
    double angle_1;
    double operator()(double phi);
};

evolute::evolute(int n_t, double add, double ded, double p_ang):
    n_teeth(n_t),
    addendum(add), dedendum(ded),
    c_modulus(2.0/n_t), 
    p_angle(p_ang), cos_p_angle(cos(p_ang)),
    off_rot(tan(p_angle) - p_angle), cos_off_rot(cos(off_rot)),sin_off_rot(sin(off_rot)),
    dedendum_radius(1.0 - c_modulus * dedendum),
    addendum_radius(1.0 + c_modulus * addendum),
    phi_at_dedendum(dedendum_radius > cos_p_angle ? radius2arg(dedendum_radius / cos_p_angle) : 0.0),
    phi_at_addendum(radius2arg(addendum_radius / cos_p_angle)),
    alpha(1.0 - dedendum_radius),
    angle_0(alpha * tan(p_angle))
{
    P(n_teeth);
    P(addendum);
    P(dedendum);
    P(c_modulus);
    P(p_angle);
    P(cos_p_angle);
    P(off_rot);
    P(cos_off_rot);
    P(sin_off_rot);
    P(dedendum_radius);
    P(addendum_radius);
    P(phi_at_dedendum);
    P(phi_at_addendum);
    P(alpha);
    P(angle_0);
}

double evolute::operator()(double phi)
{
       return phi;
}

void lc_Geardlg::processAction(Document_Interface *doc, const QString& cmd, QPointF& center)
{
    Q_UNUSED(doc);
    Q_UNUSED(cmd);
    Q_UNUSED(center);

    std::vector<Plug_VertexData> polyline;
    std::vector<QPointF> first_tooth;
    QTransform rotate_and_disp;

    /* we shall proceed by calculating the points for root radius,
     * base of tooth, n1 line segments to the pitch circle (this makes
     * possible to have a reference point on the pitch circle)
     * n2 line segments to the addendum circle.
     * The tooth face is aligned so the pitch point is aligned with the
     * X axis, so we can align gears over this reference point.
     *
     * Once we get one face, we mirror it using as the axis one quarter
     * of the pitch angular modulus (half of the tooth width) passing
     * along the origin.
     *
     * Finally, we get the complete set of teeth by rotating them by the
     * pitch angular modulus to get the whole gear */

    evolute ev(nteethBox->value(), /* number of teeth */
               addendumBox->value(), /* addendum */
               dedendumBox->value(), /* dedendum */
               M_PI / 180.0 * pressureBox->value()); /* pressure angle (converted to rad) */

    const double modulus = modulusBox->value();
    const double scale_factor = modulus / ev.c_modulus;
    const int    n1 = n1Box->value();
    const int    n2 = n2Box->value();
    const double rotation = rotateBox->value() * M_PI / 180.0;

    P(modulus);
    P(scale_factor);
    P(n1);
    P(n2);
    P(rotation);

    rotate_and_disp = rotate_and_disp
        .translate(center.x(), center.y())
        .rotateRadians(rotation);


    /* Build one tooth face */
    if (calcInterferenceBox->isChecked() && ev.cos_p_angle * ev.cos_p_angle > ev.dedendum_radius) {
        /* TODO: I'm here coding. */
        const int n3 = n3Box->value();
        const double angle_1_a = radius2arg(ev.cos_p_angle, ev.alpha); /* phi @ base */
        const double angle_1_b = radius2arg(1.0, ev.alpha); /* phi @ pitch */
        P(n3);
        P(angle_1_a);
        P(angle_1_b);
        int i;
        double phi, delta = angle_1_a / n3;
        for(i = 0, phi = 0.0; i <= n3; i++, phi -= delta) {
            const double re = scale_factor * re_evolute(phi, ev.alpha);
            const double im = scale_factor * im_evolute(phi, ev.alpha);
            const double cos_rot = cos(ev.angle_0), sin_rot = sin(ev.angle_0);
            const QPointF rot( cos_rot * re + sin_rot * im,
                              -sin_rot * re + cos_rot * im);
            first_tooth.push_back(rot);
            polyline.push_back(Plug_VertexData(rotate_and_disp.map(rot), 0.0));
        } /* for */
    } else {
        QPointF root( scale_factor * ev.dedendum_radius * ev.cos_off_rot,
                     -scale_factor * ev.dedendum_radius * ev.sin_off_rot);
        first_tooth.push_back(root);
        polyline.push_back(Plug_VertexData(rotate_and_disp.map(root), 0.0));
    }

    int i;
    const int N = n1 + n2;
    double phi;
    double delta = (ev.p_angle + ev.off_rot - ev.phi_at_dedendum) / n1;
    for (i = 0, phi = ev.phi_at_dedendum; i <= N; ++i, phi += delta) {
        QPointF point( ev.cos_p_angle * scale_factor * re_evolute(phi),
                       ev.cos_p_angle * scale_factor * im_evolute(phi)),
                rot_point( ev.cos_off_rot * point.x() + ev.sin_off_rot * point.y(),
                          -ev.sin_off_rot * point.x() + ev.cos_off_rot * point.y());
        first_tooth.push_back(rot_point);
        polyline.push_back(Plug_VertexData(rotate_and_disp.map(rot_point), 0.0));
        if (i == n1) { /* change delta to continue until we have all points */
            delta = (ev.phi_at_addendum - ev.p_angle - ev.off_rot) / n2;
        }
    } /* for */

    /* calculate the symmetric face from the original points */

    /* one half of pitch angular modulus = double of symmetry axis.
     * symmetry on an axis that passes through origin is calculated as follows:
     * x' = cos(2.0*axis_angle) * x + sin(2.0*axis_angle) * y
     * y' = sin(2.0*axis_angle) * x - cos(2.0*axis_angle) * y
     * (note: we don't use iterators as the array is growing as long as we
     * navigate it)
     */
    const double axis_angle_x_2 = M_PI / ev.n_teeth;
    const double axis_angle = axis_angle_x_2 / 2.0;
    const double cos_axis_angle_x_2 = cos(axis_angle_x_2);
    const double sin_axis_angle_x_2 = sin(axis_angle_x_2);

    /* remember size, as we don't want to duplicate next point */
    const double n_to_mirror = first_tooth.size();

    /* symmetry axis point (at top of tooth) */
    QPointF mirror_point(scale_factor * ev.addendum_radius * cos(axis_angle),
                         scale_factor * ev.addendum_radius * sin(axis_angle));
    first_tooth.push_back(mirror_point);
    polyline.push_back(Plug_VertexData(rotate_and_disp.map(mirror_point), 0.0));

    /* for all points we have to mirror (all but the last one) */
    for (i = n_to_mirror - 1; i >= 0; --i) {
        const QPointF& orig(first_tooth[i]);

        QPointF target(cos_axis_angle_x_2 * orig.x() + sin_axis_angle_x_2 * orig.y(),
                       sin_axis_angle_x_2 * orig.x() - cos_axis_angle_x_2 * orig.y());
        first_tooth.push_back(target);
        polyline.push_back(Plug_VertexData(rotate_and_disp.map(target), 0.0));
    } /* for */

    /* symmetry axis point (at interteeth) */
    QPointF mirror_point2(scale_factor * ev.dedendum_radius * cos(axis_angle + axis_angle_x_2),
                          scale_factor * ev.dedendum_radius * sin(axis_angle + axis_angle_x_2));
    first_tooth.push_back(mirror_point2);
    polyline.push_back(Plug_VertexData(rotate_and_disp.map(mirror_point2), 0.0));

    /* now, we have to rotate the tooth to get all the teeth missing. */
    for (i = 1; i < ev.n_teeth; i++) {

        const double angle = M_PI * ev.c_modulus * i;
        const double cos_angle = cos(angle);
        const double sin_angle = sin(angle);

        for (std::vector<QPointF>::iterator it = first_tooth.begin();
                it != first_tooth.end(); ++it)
        {
            const QPointF& orig = *it;
            polyline.push_back(Plug_VertexData(rotate_and_disp.map(QPointF(
                                cos_angle * orig.x() - sin_angle * orig.y(),
                                sin_angle * orig.x() + cos_angle * orig.y())),
                                               0.0));
        } /* for */
    } /* for */

    QString lastLayer = doc->getCurrentLayer();

#define LAYER(fmt) do { \
            if (useLayersBox->isChecked()) { \
                char buffer[128]; \
                snprintf(buffer, sizeof buffer, \
                        "gear_M%6.4f_" fmt, modulus); \
                doc->setLayer(buffer); \
            } \
        } while(0)

    LAYER("shapes"); 
    doc->addPolyline(polyline, true);

    if (drawPitchCircleBox->isChecked()) {
        LAYER("pitch_circles");
        doc->addCircle(&center, scale_factor);
    }

    if (drawAddendumCircleBox->isChecked()) {
        LAYER("addendums");
        doc->addCircle(&center, scale_factor * ev.addendum_radius);
    }

    if (drawRootCircleBox->isChecked()) {
        LAYER("dedendums");
        doc->addCircle(&center, scale_factor * ev.dedendum_radius);
    }

    if (drawBaseCircleBox->isChecked()) {
        LAYER("base_lines");
        doc->addCircle(&center, scale_factor * ev.cos_p_angle);
    }

    if (drawPressureLineBox->isChecked() || drawPressureLimitBox->isChecked()) {
        LAYER("action_lines");
        QPointF p1(scale_factor * cos(ev.p_angle + rotation) * ev.cos_p_angle,
                   scale_factor * sin(ev.p_angle + rotation) * ev.cos_p_angle),
                p2(scale_factor * cos(rotation),
                   scale_factor * sin(rotation));
        p1 += center; p2 += center;
        if (drawPressureLimitBox->isChecked())
            doc->addLine(&center, &p1);
        if (drawPressureLineBox->isChecked())
            doc->addLine(&p1, &p2);
    }

    if (useLayersBox->isChecked())
        doc->setLayer(lastLayer);

    writeSettings();
}

void lc_Geardlg::checkAccept()
{
    accept();
}

lc_Geardlg::~lc_Geardlg()
{
}

void lc_Geardlg::closeEvent(QCloseEvent *event)
{
    INFO("BEGIN\n");
    QWidget::closeEvent(event);
    INFO("END\n");
}


void lc_Geardlg::readSettings()
{

    INFO("BEGIN\n");

    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(430,140)).toSize();

#define R(var,toFunc, defval) do { \
        INFO(#var "Box->setValue(settings.value(\""#var"\", " #defval ")." #toFunc "());\n"); \
        var ## Box->setValue(settings.value(#var, defval).toFunc()); \
    } while(0)
        
#define RB(var,defval) do { \
        INFO(#var "Box->setChecked(settings.value(\""#var"\", " #defval ").toBool());\n"); \
        var ## Box->setChecked(settings.value(#var, defval).toBool()); \
    } while (0)
    R(rotate, toDouble,         0.0 );
    R(nteeth, toInt,           20   );
    R(modulus, toDouble,        1.0 );
    R(pressure, toDouble,      20.0 );
    R(addendum, toDouble,       1.0 );
    R(dedendum, toDouble,       1.25);
    R(n1, toInt,               16   );
    R(n2, toInt,               16   );
    RB(useLayers,            true   );
    RB(drawAddendumCircle,  false   );
    RB(drawPitchCircle,      true   );
    RB(drawBaseCircle,       true   );
    RB(drawRootCircle,      false   );
    RB(drawPressureLine,     true   );
    RB(drawPressureLimit,   false   );
    RB(calcInterference,    false   );
    R(n3, toInt,               16   );

    resize(size);
    move(pos);
    
    INFO("END\n");
}

void lc_Geardlg::writeSettings()
{
#define W(var, vfunc) do { \
        INFO("settings.setValue(\"" #var ", " #var "Box->" #vfunc "());\n"); \
        settings.setValue(#var, var##Box->vfunc()); \
    } while (0)
#define WN(var) W(var, value)
#define WB(var) W(var, isChecked)
    
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    WN(nteeth);
    WN(modulus);
    WN(pressure);
    WN(addendum);
    WN(dedendum);
    WN(n1);
    WN(n2);
    WB(useLayers);
    WB(drawAddendumCircle);
    WB(drawPitchCircle);
    WB(drawBaseCircle);
    WB(drawRootCircle);
    WB(drawPressureLine);
    WB(drawPressureLimit);
    WB(calcInterference);
    WN(n3);
}
