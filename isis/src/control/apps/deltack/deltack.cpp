#include "Isis.h"

#include "BundleAdjust.h"
#include "BundleSettings.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Distance.h"
#include "History.h"
#include "IException.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Process.h"
#include "Sensor.h"
#include "SerialNumberList.h"
#include "Table.h"

using namespace std;
using namespace Isis;

Distance GetRadius(QString filename, Latitude lat, Longitude lon);

void IsisMain() {
  // Create a serial number list
  UserInterface &ui = Application::GetUserInterface();
  QString filename = ui.GetFileName("FROM");
  SerialNumberList serialNumberList;
  serialNumberList.Add(filename);

  // Get the coordinate for updating the camera pointing
  // We will want to make the camera pointing match the lat/lon at this
  // line sample
  double samp1 = ui.GetDouble("SAMP1");
  double line1 = ui.GetDouble("LINE1");
  Latitude lat1(ui.GetDouble("LAT1"), Angle::Degrees);
  Longitude lon1(ui.GetDouble("LON1"), Angle::Degrees);
  Distance rad1;
  if(ui.WasEntered("RAD1")) {
    rad1 = Distance(ui.GetDouble("RAD1"), Distance::Meters);
  }
  else {
    rad1 = GetRadius(ui.GetFileName("FROM"), lat1, lon1);
  }

  // In order to use the bundle adjustment class we will need a control
  // network
  ControlMeasure * m = new ControlMeasure;
  m->SetCubeSerialNumber(serialNumberList.SerialNumber(0));
  m->SetCoordinate(samp1, line1);
//   m->SetType(ControlMeasure::Manual);
  m->SetType(ControlMeasure::RegisteredPixel);

  ControlPoint * p = new ControlPoint;
  p->SetAprioriSurfacePoint(SurfacePoint(lat1, lon1, rad1));
  p->SetId("Point1");
  p->SetType(ControlPoint::Fixed);
  p->Add(m);

  ControlNet cnet;
//  cnet.SetType(ControlNet::ImageToGround);
  cnet.AddPoint(p);

    // We need the target body
    Cube c;
    c.open(filename, "rw");
    //check for target name
    if(c.label()->hasKeyword("TargetName", PvlObject::Traverse)) {
//       c.Label()->findKeyword("TargetName");
      PvlGroup inst = c.label()->findGroup("Instrument", PvlObject::Traverse);
      QString targetName = inst["TargetName"];
      cnet.SetTarget(targetName);
    }
    c.close();

  // See if they wanted to solve for twist
  if(ui.GetBoolean("TWIST")) {
    double samp2 = ui.GetDouble("SAMP2");
    double line2 = ui.GetDouble("LINE2");
    Latitude lat2(ui.GetDouble("LAT2"), Angle::Degrees);
    Longitude lon2(ui.GetDouble("LON2"), Angle::Degrees);
    Distance rad2;
    if(ui.WasEntered("RAD2")) {
      rad2 = Distance(ui.GetDouble("RAD2"), Distance::Meters);
    }
    else {
      rad2 = GetRadius(ui.GetFileName("FROM"), lat2, lon2);
    }

    ControlMeasure * m = new ControlMeasure;
    m->SetCubeSerialNumber(serialNumberList.SerialNumber(0));
    m->SetCoordinate(samp2, line2);
    m->SetType(ControlMeasure::Manual);

    ControlPoint * p = new ControlPoint;
    p->SetAprioriSurfacePoint(SurfacePoint(lat2, lon2, rad2));
    p->SetId("Point2");
    p->SetType(ControlPoint::Fixed);
    p->Add(m);

    cnet.AddPoint(p);
  }

  // Bundle adjust to solve for new pointing
  try {



    // =========================================================================================//
    // ============= Use the bundle settings to initialize member variables ====================//
    // =========================================================================================//
    BundleSettings settings;
    settings.setValidateNetwork(false);

    // set the following:
    //     solve observation mode = false
    //     update cube label      = false
    //     error propagation      = false
    //     solve radius           = false
    //     latitude sigma         = 1000.0
    //     longitude sigma        = 1000.0
    //     radius sigma           = -1.0 (since we are not solving for radius)
    //     outlier rejection = false
    settings.setSolveOptions(BundleSettings::SpecialK, false, false, false, false,
                             1000.0, 1000.0, -1.0);

    // NOTE: no need to set position sigmas or solve degrees since we are not solving for any
    // position factors
    //       position option sigmas default to -1.0
    //       spkDegree = spkSolveDegree = 2
    //       solveOverHermiteSpline = false
    //       position sigma = velocity sigma = acceleration sigma = -1.0
    settings.setInstrumentPositionSolveOptions(BundleSettings::NoPositionFactors);

    // use defaults
    //       pointing option sigmas -1.0
    //       ckDegree = ckSolveDegree = 2
    //       fitOverExisting = false
    //       angle sigma = angular velocity sigma = angular acceleration sigma = -1.0
    settings.setInstrumentPointingSolveOptions(BundleSettings::AnglesOnly, ui.GetBoolean("TWIST"));

    settings.setConvergenceCriteria(BundleSettings::ParameterCorrections,
                                    ui.GetDouble("SIGMA0"),
                                    ui.GetInteger("MAXITS"));

    settings.setOutputFiles("", true, false, true);
    // ===========================================================================================//
    // =============== End Bundle Settings =======================================================//
    // ===========================================================================================//

    BundleAdjust bundleAdjust(settings, cnet, serialNumberList);
    bundleAdjust.solveCholesky();

    Cube c;
    c.open(filename, "rw");

    //check for existing polygon, if exists delete it
    if(c.label()->hasObject("Polygon")) {
      c.label()->deleteObject("Polygon");
    }

    Table cmatrix = bundleAdjust.cMatrix(0);

    // Write out a description in the spice table
    QString deltackComment = "deltackAdjusted = " + Isis::iTime::CurrentLocalTime();
    cmatrix.Label().addComment(deltackComment);
    //PvlKeyword description("Description");
    //description = "Camera pointing updated via deltack application";
    //cmatrix.Label().findObject("Table",Pvl::Traverse).addKeyword(description);

    // Update the cube history
    History hist = History("IsisCube");
    try {
      // read history from cube, if it exists.
      c.read(hist);
    }
    catch(IException &e) {
      // if the history does not exist in the cube, the cube's write method will add it.
    }
      c.write(cmatrix); 
      hist.AddEntry();
      c.write(hist);

    // clean up
    c.close();

    PvlGroup gp("DeltackResults");
    gp += PvlKeyword("Status", "Camera pointing updated");
    Application::Log(gp);
  }
  catch(IException &e) {
    QString msg = "Unable to update camera pointing for [" + filename + "]";
    throw IException(e, IException::Unknown, msg, _FILEINFO_);
  }

}

// Compute the radius at the lat/lon
Distance GetRadius(QString filename, Latitude lat, Longitude lon) {
  Cube cube(filename, "r");
  Sensor sensor(cube);
  sensor.SetGround(SurfacePoint(lat, lon, sensor.LocalRadius(lat, lon)));
  Distance radius = sensor.LocalRadius();
  if(!radius.isValid()) {
    QString msg = "Could not determine radius from DEM at lat/lon [";
    msg += toString(lat.degrees()) + "," + toString(lon.degrees()) + "]";
    throw IException(IException::Unknown, msg, _FILEINFO_);
  }
  return radius;
}

