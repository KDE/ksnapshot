void WindowGrabber::platformSetup(QPixmap &pm, QRect &geom)
{
    Q_UNUSED(pm)
    Q_UNUSED(geom)
    qWarning() << tr("Not implemented yet");
}

QPixmap WindowGrabber::grabCurrent(bool includeDecorations)
{
    Q_UNUSED(includeDecorations)
    qWarning() << tr("Not implemented yet");
    return QPixmap();
}