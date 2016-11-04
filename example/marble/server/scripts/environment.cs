//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

datablock AudioProfile(HeavyRainSound)
{
   filename    = "~/data/sound/environment/ambient/rain.ogg";
   description = AudioLooping2d;
};

datablock PrecipitationData(HeavyRain)
{
   soundProfile = "HeavyRainSound";

   dropTexture = "~/data/environment/rain";
   splashTexture = "~/data/environment/water_splash";
   dropSize = 0.75;
   splashSize = 0.2;
   useTrueBillboards = false;
   splashMS = 250;
};

 //-----------------------------------------------------------------------------

datablock AudioProfile(ThunderCrash1Sound)
{
   filename  = "~/data/sound/environment/ambient/thunder1.ogg";
   description = Audio2d;
};

datablock AudioProfile(ThunderCrash2Sound)
{
   filename  = "~/data/sound/environment/ambient/thunder2.ogg";
   description = Audio2d;
};

datablock AudioProfile(ThunderCrash3Sound)
{
   filename  = "~/data/sound/environment/ambient/thunder3.ogg";
   description = Audio2d;
};

datablock AudioProfile(ThunderCrash4Sound)
{
   filename  = "~/data/sound/environment/ambient/thunder4.ogg";
   description = Audio2d;
};

//datablock AudioProfile(LightningHitSound)
//{
//   filename  = "~/data/sound/crossbow_explosion.ogg";
//   description = AudioLightning3d;
//};

datablock LightningData(LightningStorm)
{
   strikeTextures[0]  = "demo/data/environment/lightning1frame1";
   strikeTextures[1]  = "demo/data/environment/lightning1frame2";
   strikeTextures[2]  = "demo/data/environment/lightning1frame3";
   
   //strikeSound = LightningHitSound;
   thunderSounds[0] = ThunderCrash1Sound;
   thunderSounds[1] = ThunderCrash2Sound;
   thunderSounds[2] = ThunderCrash3Sound;
   thunderSounds[3] = ThunderCrash4Sound;
};

