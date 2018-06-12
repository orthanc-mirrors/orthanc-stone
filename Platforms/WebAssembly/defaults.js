var isPendingRedraw = false;

function ScheduleRedraw()
{
  if (!isPendingRedraw) {
    isPendingRedraw = true;
    //console.log('Scheduling a refresh of the viewport, as its content changed');
    window.requestAnimationFrame(function() {
      isPendingRedraw = false;
      viewport.Redraw();
    });
  }
}
